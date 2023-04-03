/*-----------------------------------------------------------------------------/
 /	!nso - Appended XUsbPs Host Mode										   /
 /-----------------------------------------------------------------------------/
 /	Bowie Gian
 /	04/03/2023
 /	usb.c
 /
 /	Referenced XUsbPs v2.6 Library
 /	I added the missing functions to use USB host mode with the XUsbPs library.
 /----------------------------------------------------------------------------*/

#include <stdlib.h>
#include "usb.h"
#include <stdlib.h>

static u8 Buffer[MEMORY_SIZE] __attribute__((aligned(32)));

/*--------------------------------------------------------------*/
/* My Functions													*/
/*--------------------------------------------------------------*/
static void USB_qTDActivateSetup(XUsbPs_qTD *qTD, bool isIOC) {
	XUsbPs_dTDInvalidateCache(qTD);

	u32 token = (8) << 16; // Set max length

	if (isIOC)
		token |= XUSBPS_qTDTOKEN_IOC_MASK;

	// Set Active, Cerr = 3 and SETUP
	token |= XUSBPS_qTDTOKEN_ACTIVE_MASK | XUSBPS_qTDTOKEN_CERR_MASK | XUSBPS_qTDTOKEN_PID_SETUP;

	XUsbPs_WritedTD(qTD, XUSBPS_qTDTOKEN, token);

	XUsbPs_dTDFlushCache(qTD);
}

static void USB_SendSetupPacket(XUsbPs_QH *QueueHead, bool isIOC, u8  bmRequestType, u8  bRequest, u16 wValue, u16 wIndex, u16 wLength) {
	XUsbPs_qTD *qTD = QueueHead->qTDHead;

	// Reset buffer pointer
	XUsbPs_SetupData *buffSetup = (XUsbPs_SetupData *)(XUsbPs_ReaddTD(qTD, XUSBPS_qTDANLP) - 1);
	XUsbPs_WritedTD(qTD, XUSBPS_qTDBPTR0, buffSetup);
	XUsbPs_dTDFlushCache(qTD); // Flush after write (for controller)

	buffSetup->bmRequestType = bmRequestType;
	buffSetup->bRequest = bRequest;
	buffSetup->wValue = wValue;
	buffSetup->wIndex = wIndex;
	buffSetup->wLength = wLength;

	// Must flush cache after modifying buffers (for controller)
	Xil_DCacheFlushRange((unsigned int)buffSetup, sizeof(XUsbPs_SetupData));

	QueueHead->qTDHead = XUsbPs_dTDGetNLP(QueueHead->qTDHead);
	USB_qTDActivateSetup(qTD, isIOC);
}

XUsbPs_qTD* USB_qTDActivateIn(XUsbPs_QH *QueueHead, bool isIOC, bool dataNum) {
	XUsbPs_qTD *qTD = QueueHead->qTDHead;
	XUsbPs_dTDInvalidateCache(qTD);

	u32 token = 0;
	// If DATA1, toggle the data bit.  DATA0 is already 0.
	if (dataNum)
		token = XUSBPS_qTDTOKEN_DT_MASK;

	if (isIOC)
		token |= XUSBPS_qTDTOKEN_IOC_MASK;

	// Set Active and Cerr = 3, PID IN and Interrupt On Complete
	token |= XUSBPS_qTDTOKEN_ACTIVE_MASK | XUSBPS_qTDTOKEN_CERR_MASK | XUSBPS_qTDTOKEN_PID_IN;
	XUsbPs_WritedTD(qTD, XUSBPS_qTDTOKEN, token);

	// Set Max Input Transfer length
	u32 length = QueueHead->BufSize;

	XUsbPs_WritedTD(qTD, XUSBPS_qTDTOKEN,
			(XUsbPs_ReaddTD(qTD, XUSBPS_qTDTOKEN) & ~XUSBPS_dTDTOKEN_LEN_MASK) | (length << 16));

	XUsbPs_WritedTD(qTD, XUSBPS_qTDBPTR0, XUsbPs_ReaddTD(qTD, XUSBPS_qTDANLP) - 1);

	XUsbPs_dTDFlushCache(qTD);

	//Flush IN Buffer
	Xil_DCacheFlushRange((unsigned int)XUsbPs_ReaddTD(qTD, XUSBPS_qTDBPTR0), length);

	QueueHead->qTDHead = XUsbPs_dTDGetNLP(QueueHead->qTDHead);

	return qTD;
}

static void USB_PrintUTF16(char *message, int length) {
	char output[length/2];

	int outputIndex = 0;
	for (int i = 0; i < length; i++) {
		if (i < 2)
			continue;
		if ((i % 2) == 0) {
			output[outputIndex] = message[i];
			outputIndex++;
		}
	}
	output[outputIndex] = '\0';

	xil_printf("%s", output);
}

XUsbPs_qTD *USB_SetupPolling(UsbWithHost *usbWithHostInstancePtr) {
	XUsbPs *UsbInstancePtr = &usbWithHostInstancePtr->usbInstance;

	// Setup Polling
	XUsbPs_QH *pollQH = &usbWithHostInstancePtr->HostConfig.QueueHead[1];
	XUsbPs_dQHInvalidateCache(pollQH->pQH);

	// Set EP 1
	XUsbPs_WritedQH(pollQH->pQH, XUSBPS_QHCFG0, XUsbPs_ReaddQH(pollQH->pQH, XUSBPS_QHCFG0) | 0x100);
	// Clear Control EP Bit
	XUsbPs_WritedQH(pollQH->pQH, XUSBPS_QHCFG0, XUsbPs_ReaddQH(pollQH->pQH, XUSBPS_QHCFG0) & ~0x08000000);
	XUsbPs_WritedQH(pollQH->pQH, XUSBPS_QHCFG1, 0x40001001);


	XUsbPs_dQHFlushCache(pollQH->pQH);

	XUsbPs_qTD *qTDReceiver = USB_qTDActivateIn(pollQH, true, 0);

	// Set Frame List to 8
	XUsbPs_SetBits(UsbInstancePtr, XUSBPS_CMD_OFFSET, XUSBPS_CMD_FS01_MASK | XUSBPS_CMD_FS2_MASK);

	// Setup Frame List according to polling rate
	u32 *frameListPtr = (u32 *)0x1000;
	for (int i = 0; i < 8; i++) {
		if ((i % POLLING_MS) == 0)
			frameListPtr[i] = 0x842;
		else
			frameListPtr[i] = 0x001;
	}
	Xil_DCacheFlushRange((unsigned int)frameListPtr, sizeof(u32) * 8);
	// Set Frame List Address
	XUsbPs_WriteReg(UsbInstancePtr->Config.BaseAddress, XUSBPS_LISTBASE_OFFSET, (u32)frameListPtr);
	return qTDReceiver;
}

static int strNumManu = 0;
static int strNumDesc = 0;
static int strNumSerial = 0;
static int length = 0;
static XUsbPs_qTD *qTDReceiver = 0;

bool USB_SetupDevice(UsbWithHost *usbWithHostInstancePtr, int status) {
	bool isSetup = false;
	XUsbPs_QH *setupQH = &usbWithHostInstancePtr->HostConfig.QueueHead[0];
	u8 *buffInput = 0;

	XUsbPs_dQHInvalidateCache(setupQH->pQH);

	if (status > 0 && status < 10) {
		XUsbPs_dTDInvalidateCache(qTDReceiver); // Invalidate before CPU read ??output buff??~~~~~~~~~~~~~~~~~~~~
		buffInput = (u8 *)XUsbPs_ReaddTD(qTDReceiver, XUSBPS_qTDANLP) - 1;
	}

	if (status == 0) { // Request device descriptor
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0100, 0, 0x12);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 1) {
		// Read string locations
		strNumManu = buffInput[14];
		strNumDesc = buffInput[15];
		strNumSerial = buffInput[16];

		// Request Device String Info (0x0409 is English)
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0300 + strNumDesc, 0x0409, 0x02);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 2) {
		// Get string length
		length = buffInput[0];

		// Request Device String (0x0409 is English)
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0300 + strNumDesc, 0x0409, length);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 3) {
		xil_printf("Device: ");
		USB_PrintUTF16((char *)buffInput, length);
		xil_printf("\r\n");

		// Request Manufacturer String Info
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0300 + strNumManu, 0x0409, 0x02);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 4) {
		length = buffInput[0];

		// Request Manufacturer String
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0300 + strNumManu, 0x0409, length);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 5) {
		xil_printf("Manufacturer: ");
		USB_PrintUTF16((char *)buffInput, length);
		xil_printf("\r\n");

		// Request Serial Number Info
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0300 + strNumSerial, 0x0409, 0x02);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 6) {
		length = buffInput[0];

		// Request Serial Number
		USB_SendSetupPacket(setupQH, false, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0300 + strNumSerial, 0x0409, length);
		qTDReceiver = USB_qTDActivateIn(setupQH, true, 1);
	} else if (status == 7) {
		int num1 = buffInput[2];
		int num2 = buffInput[3];
		xil_printf("Serial Number: %d%d\r\n", num1, num2);

		// Set Config 1
		USB_SendSetupPacket(setupQH, true, 0x00, XUSBPS_REQ_SET_CONFIGURATION, 0x0001, 0x0000, 0x0000);
	} else if (status == 8) {
		// Set HID Idle Time (24ms)
		USB_SendSetupPacket(setupQH, true, 0x21, 0x0A, 0x0600, 0x0000, 0x0000);
	} else if (status == 9) {
		// Set HID Protocol 0
		USB_SendSetupPacket(setupQH, true, 0x21, 0x0B, 0x0001, 0x0000, 0x0000);
	} else if (status == 10) {
		xil_printf("Setup successful!\r\n");
		isSetup = true;
	}

	XUsbPs_dQHFlushCache(setupQH->pQH);
	return isSetup;
}

/*--------------------------------------------------------------*/
/* Modified Functions from XUsbPs Library						*/
/*--------------------------------------------------------------*/

/*****************************************************************************/
/**
 *
 * This function associates a buffer with a Transfer Descriptor. The function
 * will take care of splitting the buffer into multiple 4kB aligned segments if
 * the buffer happens to span one or more 4kB pages.
 *
 * @param	dTDIndex is a pointer to the Transfer Descriptor
 * @param	BufferPtr is pointer to the buffer to link to the descriptor.
 * @param	BufferLen is the length of the buffer.
 *
 * @return
 *		- XST_SUCCESS: The operation completed successfully.
 *		- XST_FAILURE: An error occurred.
 *		- XST_USB_BUF_TOO_BIG: The provided buffer is bigger than tha
 *		maximum allowed buffer size (16k).
 *
 * @note
 * 		Cache invalidation and flushing needs to be handler by the
 * 		caller of this function.
 *
 ******************************************************************************/
static int XUsbPs_qTDAttachBuffer(XUsbPs_qTD *qTDPtr,
					const u8 *BufferPtr, u32 BufferLen)
{
	u32	BufAddr;
	u32	BufEnd;
	u32	PtrNum;

	Xil_AssertNonvoid(qTDPtr    != NULL);

	/* Check if the buffer is smaller than 16kB. */
	if (BufferLen > XUSBPS_dTD_BUF_MAX_SIZE) {
		return XST_USB_BUF_TOO_BIG;
	}

	/* Get a u32 of the buffer pointer to avoid casting in the following
	 * logic operations.
	 */
	BufAddr = (u32) BufferPtr;


	/* Set the buffer pointer 0. Buffer pointer 0 can point to any location
	 * in memory. It does not need to be 4kB aligned. However, if the
	 * provided buffer spans one or more 4kB boundaries, we need to set up
	 * the subsequent buffer pointers which must be 4kB aligned.
	 */
	XUsbPs_WritedTD(qTDPtr, XUSBPS_qTDBPTR(0), BufAddr);

	/* Check if the buffer spans a 4kB boundary.
	 *
	 * Only do this check, if we are not sending a 0-length buffer.
	 */
	if (BufferLen > 0) {
		BufEnd = BufAddr + BufferLen -1;
		PtrNum = 1;

		while ((BufAddr & 0xFFFFF000) != (BufEnd & 0xFFFFF000)) {
			/* The buffer spans at least one boundary, let's set
			 * the next buffer pointer and repeat the procedure
			 * until the end of the buffer and the pointer written
			 * are in the same 4kB page.
			 */
			BufAddr = (BufAddr + 0x1000) & 0xFFFFF000;
			XUsbPs_WritedTD(qTDPtr, XUSBPS_qTDBPTR(PtrNum),
								BufAddr);
			PtrNum++;
		}
	}

	/* Set the length of the buffer. */
	XUsbPs_qTDSetTransferLen(qTDPtr, BufferLen);

	/* We remember the buffer pointer in the alternate pointer field
	 * making sure it is disabled. This makes it easier to reset the buffer pointer
	 * after a buffer has been received on the endpoint. The buffer pointer
	 * needs to be reset because the DMA engine modifies the buffer pointer
	 * while receiving.
	 */
	XUsbPs_WritedTD(qTDPtr, XUSBPS_qTDANLP, BufferPtr + 1);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 * This function set the Max PacketLen for the queue head for isochronous EP.
 *
 * If the max packet length is greater than XUSBPS_MAX_PACKET_SIZE, then
 * Mult bits are set to reflect that.
 *
 * @param	pQH is a pointer to the QH element.
 * @param	Len is the Length to be set.
 *
 ******************************************************************************/
static void XUsbPs_pQHSetMaxPacketLen(XUsbPs_pQH *pQH, u32 Len)
{
	u32 Mult = (Len & ENDPOINT_MAXP_MULT_MASK) >> ENDPOINT_MAXP_MULT_SHIFT;
	u32 MaxPktSize = (Mult > 1) ? ENDPOINT_MAXP_LENGTH : Len;

	if (MaxPktSize > XUSBPS_MAX_PACKET_SIZE) {
		return;
	}

	if (Mult > 3) {
		return;
	}

	/* Set Max packet size */
	XUsbPs_WritedQH(pQH, XUSBPS_QHCFG0,
		(XUsbPs_ReaddQH(pQH, XUSBPS_QHCFG0) &
			~XUSBPS_QHCFG0_MPL_MASK) |
			(MaxPktSize << XUSBPS_QHCFG0_MPL_SHIFT));

	/* Set Mult to tell hardware how many transactions in each microframe */
//	XUsbPs_WritedQH(pQH, XUSBPS_QHCFG1,
//		(XUsbPs_ReaddQH(pQH, XUSBPS_QHCFG1) &
//			~XUSBPS_QHCFG1_MULT_MASK) |
//			(Mult << XUSBPS_QHCFG1_MULT_SHIFT));

}

/*****************************************************************************/
/**
*
* This function initializes the queue head pointer data structure.
*
* The function sets up the local data structure with the aligned addresses for
* the Queue Head and Transfer Descriptors.
*
* @param	DevCfgPtr is pointer to the XUsbPs DEVICE configuration
*		structure.
*
* @return	none
*
* @note
* 		Endpoints of type XUSBPS_EP_TYPE_NONE are not used in the
* 		system. Therefore no memory is reserved for them.
*
******************************************************************************/
static void XUsbPs_QHListInit(XUsbPs_HostConfig *HostCfgPtr)
{
	int	QHNum;
	u8	*p;

	XUsbPs_QH	*QueueHead;

	/* Set up the XUsbPs_Endpoint array. This array is used to define the
	 * location of the Queue Head list and the Transfer Descriptors in the
	 * block of DMA memory that has been passed into the driver.
	 *
	 * 'p' is used to set the pointers in the local data structure.
	 * Initially 'p' is pointed to the beginning of the DMAable memory
	 * block. As pointers are assigned, 'p' is incremented by the size of
	 * the respective object.
	 */
	QueueHead	= HostCfgPtr->QueueHead;

	/* Start off with 'p' pointing to the (aligned) beginning of the DMA
	 * buffer.
	 */
	p = (u8 *) HostCfgPtr->PhysAligned;


	/* Initialize the Queue Head pointer list.
	 *
	 * Each endpoint has two Queue Heads. One for the OUT direction and one
	 * for the IN direction. An OUT Queue Head is always followed by an IN
	 * Queue Head.
	 *
	 * Queue Head alignment is XUSBPS_dQH_ALIGN.
	 *
	 * Note that we have to reserve space here for unused endpoints.
	 */
	for (QHNum = 0; QHNum < HostCfgPtr->NumQHs; ++QHNum) {
		QueueHead[QHNum].pQH = (XUsbPs_pQH *) p;
		p += XUSBPS_dQH_ALIGN;
	}


	/* 'p' now points to the first address after the Queue Head list. The
	 * Transfer Descriptors start here.
	 *
	 * Each endpoint has a variable number of Transfer Descriptors
	 * depending on user configuration.
	 *
	 * Transfer Descriptor alignment is XUSBPS_dTD_ALIGN.
	 */
	for (QHNum = 0; QHNum < HostCfgPtr->NumQHs; ++QHNum) {
		QueueHead[QHNum].qTDs		= (XUsbPs_qTD *) p;
		QueueHead[QHNum].qTDHead	= (XUsbPs_qTD *) p;
		QueueHead[QHNum].qTDTail	= (XUsbPs_qTD *) p;
		p += XUSBPS_dTD_ALIGN * QueueHead[QHNum].NumBufs;
	}


	/* 'p' now points to the first address after the Transfer Descriptors.
	 * The data buffers for the OUT Transfer Descriptors start here.
	 *
	 * Note that IN (TX) Transfer Descriptors are not assigned buffers at
	 * this point. Buffers will be assigned when the user calls the send()
	 * function.
	 */
	for (QHNum = 0; QHNum < HostCfgPtr->NumQHs; ++QHNum) {
		/* If BufSize for this endpoint is set to 0 it means
		 * that we do not need to attach a buffer to this
		 * descriptor. We also initialize it's buffer pointer
		 * to NULL.
		 */
		if (0 == QueueHead[QHNum].BufSize) {
			QueueHead[QHNum].qTDBufs = NULL;
			continue;
		}

		QueueHead[QHNum].qTDBufs = p;
		p += QueueHead[QHNum].BufSize * QueueHead[QHNum].NumBufs;
	}


	/* Initialize the endpoint event handlers to NULL.
	 */
	for (QHNum = 0; QHNum < HostCfgPtr->NumQHs; ++QHNum) {
		QueueHead[QHNum].HandlerFunc = NULL;
		QueueHead[QHNum].HandlerIsoFunc = NULL;
	}
}

/*****************************************************************************/
/**
*
* This function initializes the Queue Head List in memory.
*
* @param	HostCfgPtr is a pointer to the XUsbPs HOST configuration
*		structure.
*
* @return	None
*
* @note		None.
*
******************************************************************************/
static void XUsbPs_QHInit(XUsbPs_HostConfig *HostCfgPtr)
{
	int	QHNum;

	XUsbPs_QH	*QueueHead;

	/* Setup pointers for simpler access. */
	QueueHead	= HostCfgPtr->QueueHead;


	/* Go through the list of Queue Head entries and:
	 *
	 * - Set Transfer Descriptor addresses
	 * - Set Maximum Packet Size
	 *
	 */
	for (QHNum = 0; QHNum < HostCfgPtr->NumQHs; ++QHNum) {


		if (QHNum == 0) {			// Temp config: EPS=FS EpNum=0 Addr=0 H=1 (Head) DTC=1
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHHLPTR, 0x802);
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHCFG0, 0xF800C000);
		} else if (QHNum == 1) {	// Temp config: EPS=FS EpNum=0 Addr=0 H=0 (Head) DTC=1
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHHLPTR, 0x842);
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHCFG0, 0xF8004000);
		} else if (QHNum == 2) {	// Temp config: EPS=FS EpNum=0 Addr=0 H=0 (Head) DTC=1
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHHLPTR, 0x8C2);
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHCFG0, 0xF8004000);
		} else {
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHHLPTR, 0x802);
			XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHCFG0, 0xF8004000);
		}

		XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHCFG1, 0x40000000);

		//XUsbPs_WritedQH(QueueHead[QHNum].pQH, XUSBPS_QHCPTR, QueueHead[QHNum].qTDs);

		XUsbPs_pQHSetMaxPacketLen(QueueHead[QHNum].pQH, QueueHead[QHNum].MaxPacketSize);


		/* Set up the overlay next dTD pointer. */
		XUsbPs_WritedQH(QueueHead[QHNum].pQH,
				XUSBPS_QHqTDNLP, QueueHead[QHNum].qTDs);

		XUsbPs_dQHFlushCache(QueueHead[QHNum].pQH);
	}
}

/*****************************************************************************/
/**
 *
 * This function initializes the Transfer Descriptors lists in memory.
 *
 * @param	DevCfgPtr is a pointer to the XUsbPs DEVICE configuration
 *		structure.
 *
 * @return
 *		- XST_SUCCESS: The operation completed successfully.
 *		- XST_FAILURE: An error occurred.
 *
 ******************************************************************************/
static int XUsbPs_qTDInit(XUsbPs_HostConfig *HostCfgPtr)
{
	int	QHNum;

	XUsbPs_QH	*QueueHead;

	/* Setup pointers for simpler access. */
	QueueHead	= HostCfgPtr->QueueHead;


	/* Walk through the list of endpoints and initialize their Transfer
	 * Descriptors.
	 */
	for (QHNum = 0; QHNum < HostCfgPtr->NumQHs; ++QHNum) {
		int	Td;
		int	NumdTD;

		/* + Set the next link pointer
		 * + Set the interrupt complete and the active bit
		 * + Attach the buffer to the dTD
		 */
		NumdTD = QueueHead[QHNum].NumBufs;

		for (Td = 0; Td < NumdTD; ++Td) {
			int	Status;

			int NextTd = (Td + 1) % NumdTD;

			XUsbPs_dTDInvalidateCache(&QueueHead[QHNum].qTDs[Td]);

			/* Set NEXT link pointer. */
			XUsbPs_WritedTD(&QueueHead[QHNum].qTDs[Td], XUSBPS_qTDNLP,
					&QueueHead[QHNum].qTDs[NextTd]);

			// Disable Alt NLP
			XUsbPs_WritedTD(&QueueHead[QHNum].qTDs[Td], XUSBPS_qTDANLP,
					0x1);

			// PID Setup
			XUsbPs_WritedTD(&QueueHead[QHNum].qTDs[Td], XUSBPS_qTDTOKEN,
					0x200);

			/* Set the OUT descriptor ACTIVE and enable the
			 * interrupt on complete.
			 */
			// wrong loc for qTD
//			XUsbPs_dTDSetActive(&QueueHead[QHNum].qTDs[Td]);
//			XUsbPs_dTDSetIOC(&QueueHead[QHNum].qTDs[Td]);


			/* Set up the data buffer with the descriptor. If the
			 * buffer pointer is NULL it means that we do not need
			 * to attach a buffer to this descriptor.
			 */
			if (NULL == QueueHead[QHNum].qTDBufs) {
				XUsbPs_dTDFlushCache(&QueueHead[QHNum].qTDs[Td]);
				continue;
			}

			Status = XUsbPs_qTDAttachBuffer(
					&QueueHead[QHNum].qTDs[Td],
					QueueHead[QHNum].qTDBufs +
						(Td * QueueHead[QHNum].BufSize),
					QueueHead[QHNum].BufSize);
			if (XST_SUCCESS != Status) {
				return XST_FAILURE;
			}

			XUsbPs_dTDFlushCache(&QueueHead[QHNum].qTDs[Td]);
		}
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 *
 * This function configures the HOST side of the controller. The caller needs
 * to pass in the desired configuration (e.g. number of endpoints) and a
 * DMAable buffer that will hold the Queue Head List and the Transfer
 * Descriptors. The required size for this buffer can be obtained by the caller
 * using the: XUsbPs_DeviceMemRequired() macro.
 *
 * @param	InstancePtr is a pointer to the XUsbPs instance of the
 *		controller.
 * @param	CfgPtr is a pointer to the configuration structure that contains
 *		the desired DEVICE side configuration.
 *
 * @return
 *		- XST_SUCCESS: The operation completed successfully.
 *		- XST_FAILURE: An error occurred.
 *
 * @note
 * 		The caller may configure the controller for both, DEVICE and
 * 		HOST side.
 *
 ******************************************************************************/
int XUsbPs_ConfigureHost(UsbWithHost *usbWithHostInstancePtr,
			    const XUsbPs_HostConfig *CfgPtr)
{
	XUsbPs *InstancePtr = &usbWithHostInstancePtr->usbInstance;

	int	Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CfgPtr      != NULL);

	/* Copy the configuration data over into the local instance structure */
	usbWithHostInstancePtr->HostConfig = *CfgPtr;


	/* Align the buffer to a 2048 byte (XUSBPS_dQH_BASE_ALIGN) boundary.*/
	usbWithHostInstancePtr->HostConfig.PhysAligned =
		(InstancePtr->DeviceConfig.DMAMemPhys +
					 XUSBPS_dQH_BASE_ALIGN) &
						~(XUSBPS_dQH_BASE_ALIGN -1);

	/* Initialize the endpoint pointer list data structure. */
	XUsbPs_QHListInit(&usbWithHostInstancePtr->HostConfig);


	/* Initialize the Queue Head structures in DMA memory. */
	XUsbPs_QHInit(&usbWithHostInstancePtr->HostConfig);


	/* Initialize the Transfer Descriptors in DMA memory.*/
	Status = XUsbPs_qTDInit(&usbWithHostInstancePtr->HostConfig);
	if (XST_SUCCESS != Status) {
		return XST_FAILURE;
	}

	/* Changing the HOST mode requires a controller RESET. */
	if (XST_SUCCESS != XUsbPs_Reset(InstancePtr)) {
		return XST_FAILURE;
	}

	/* Set the Queue Head List address. */
	XUsbPs_WriteReg(InstancePtr->Config.BaseAddress,
				XUSBPS_ASYNCLISTADDR_OFFSET,
		usbWithHostInstancePtr->HostConfig.PhysAligned);

	/* Set the USB mode register to configure HOST mode. */
	XUsbPs_WriteReg(InstancePtr->Config.BaseAddress,
				XUSBPS_MODE_OFFSET, XUSBPS_MODE_CM_HOST_MASK);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur for
* the USB controller. This function is application specific since the actual
* system may or may not have an interrupt controller. The USB controller could
* be directly connected to a processor without an interrupt controller.  The
* user should modify this function to fit the application.
*
* @param	IntcInstancePtr is a pointer to instance of the Intc controller.
* @param	UsbInstancePtr is a pointer to instance of the USB controller.
* @param	UsbIntrId is the Interrupt Id and is typically
* 		XPAR_<INTC_instance>_<USB_instance>_VEC_ID value
* 		from xparameters.h
*
* @return
* 		- XST_SUCCESS if successful
* 		- XST_FAILURE on error
*
******************************************************************************/
int UsbSetupIntrSystem(XScuGic *IntcInstancePtr,
			      XUsbPs *UsbInstancePtr, u16 UsbIntrId)
{
	int Status;
	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}
	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Xil_ExceptionInit();
	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				    (Xil_ExceptionHandler)XScuGic_InterruptHandler,
				    IntcInstancePtr);
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, UsbIntrId,
				(Xil_ExceptionHandler)XUsbPs_IntrHandler,
				(void *)UsbInstancePtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}
	/*
	 * Enable the interrupt for the device.
	 */
	XScuGic_Enable(IntcInstancePtr, UsbIntrId);

	/*
	 * Enable interrupts in the Processor.
	 */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function disables the interrupts that occur for the USB controller.
*
* @param	IntcInstancePtr is a pointer to instance of the INTC driver.
* @param	UsbIntrId is the Interrupt Id and is typically
* 		XPAR_<INTC_instance>_<USB_instance>_VEC_ID value
* 		from xparameters.h
*
* @return	None
*
* @note		None.
*
******************************************************************************/
void UsbDisableIntrSystem(XScuGic *IntcInstancePtr, u16 UsbIntrId)
{
	/* Disconnect and disable the interrupt for the USB controller. */
	XScuGic_Disconnect(IntcInstancePtr, UsbIntrId);
}

// based on UsbIntrExample() function
int USB_Setup(XScuGic *IntcInstancePtr, UsbWithHost *usbWithHostInstancePtr,
					u16 UsbDeviceId, u16 UsbIntrId, void *UsbIntrHandler)
{
	XUsbPs *UsbInstancePtr = &usbWithHostInstancePtr->usbInstance;

	int	Status;
	u8	*MemPtr = NULL;
	int	ReturnStatus = XST_FAILURE;

	const u8 NumQHs = 2;

	XUsbPs_Config		*UsbConfigPtr;
	XUsbPs_HostConfig	HostConfig;

	/* Initialize the USB driver so that it's ready to use,
	 * specify the controller ID that is generated in xparameters.h
	 */
	UsbConfigPtr = XUsbPs_LookupConfig(UsbDeviceId);
	if (NULL == UsbConfigPtr) {
		return ReturnStatus;
	}

	/* We are passing the physical base address as the third argument
	 * because the physical and virtual base address are the same in our
	 * example.  For systems that support virtual memory, the third
	 * argument needs to be the virtual base address.
	 */
	Status = XUsbPs_CfgInitialize(UsbInstancePtr,
				       UsbConfigPtr,
				       UsbConfigPtr->BaseAddress);
	if (XST_SUCCESS != Status) {
		return ReturnStatus;
	}

	/* Set up the interrupt subsystem.
	 */
	Status = UsbSetupIntrSystem(IntcInstancePtr,
				    UsbInstancePtr,
				    UsbIntrId);
	if (XST_SUCCESS != Status)
	{
		return ReturnStatus;
	}

	HostConfig.QueueHead[0].NumBufs			= 2;
	HostConfig.QueueHead[0].BufSize			= 64;
	HostConfig.QueueHead[0].MaxPacketSize	= 64;

	HostConfig.QueueHead[1].NumBufs			= 2;
	HostConfig.QueueHead[1].BufSize			= 8;
	HostConfig.QueueHead[1].MaxPacketSize	= 16;

	HostConfig.NumQHs = NumQHs;

	MemPtr = (u8 *)&Buffer[0];
	memset(MemPtr,0,MEMORY_SIZE);
	Xil_DCacheFlushRange((unsigned int)MemPtr, MEMORY_SIZE);

	HostConfig.DMAMemPhys = (u32) MemPtr;

	Status = XUsbPs_ConfigureHost(usbWithHostInstancePtr, &HostConfig);
	if (XST_SUCCESS != Status) {
		return ReturnStatus;
	}

	// Set handler for interrupts
	Status = XUsbPs_IntrSetHandler(UsbInstancePtr, UsbIntrHandler, NULL, XUSBPS_IXR_UI_MASK | XUSBPS_IXR_PC_MASK);
	if (XST_SUCCESS != Status) {
		return ReturnStatus;
	}

	// Enable the interrupts
	XUsbPs_IntrEnable(UsbInstancePtr,
			XUSBPS_IXR_UR_MASK | XUSBPS_IXR_UI_MASK | XUSBPS_IXR_PC_MASK);

	// ID Input Not Sampled
	XUsbPs_ClrBits(UsbInstancePtr, XUSBPS_OTGCSR_OFFSET, XUSBPS_OTGSC_IDPU_MASK);

	// Disable Periodic Schedule
	XUsbPs_ClrBits(UsbInstancePtr, XUSBPS_CMD_OFFSET, XUSBPS_CMD_PSE_MASK);

	// Enable Async
	//XUsbPs_SetBits(UsbInstancePtr, XUSBPS_CMD_OFFSET, XUSBPS_CMD_ASE_MASK);

	// Power Enable
	XUsbPs_SetBits(UsbInstancePtr, XUSBPS_PORTSCR1_OFFSET, XUSBPS_PORTSCR_PP_MASK);

	// Read ULPI Register 0x0A
	XUsbPs_ClrBits(UsbInstancePtr, XUSBPS_ULPIVIEW_OFFSET, XUSBPS_ULPIVIEW_RW_MASK);
	XUsbPs_SetBits(UsbInstancePtr, XUSBPS_ULPIVIEW_OFFSET, XUSBPS_ULPIVIEW_RUN_MASK | 0x000A0000);

	u8 value = (XUsbPs_ReadReg(UsbInstancePtr->Config.BaseAddress, XUSBPS_ULPIVIEW_OFFSET) & XUSBPS_ULPIVIEW_DATRD_MASK) >> 8;

	// Check Power Enable Bit
	if (value & 0x40) {
		xil_printf("DEBUG: ULPI Power Bit is on.\r\n");
	} else {
		xil_printf("DEBUG: ULPI Power Bit is off. Manually turning it on.\r\n");
		XUsbPs_SetBits(UsbInstancePtr, XUSBPS_ULPIVIEW_OFFSET,
				XUSBPS_ULPIVIEW_RUN_MASK | XUSBPS_ULPIVIEW_RW_MASK | value | 0x40);
	}
	// 0x480A0000 read for E6
	// 0x680A00E6 write for power

	/* Start the USB engine */
	XUsbPs_Start(UsbInstancePtr);

	/* Set return code to indicate success and fall through to clean-up
	 * code.
	 */
	ReturnStatus = XST_SUCCESS;
	return ReturnStatus;
}

void USB_CleanUp(XScuGic *IntcInstancePtr, XUsbPs *UsbInstancePtr, u16 UsbIntrId) {
	/* Clean up. It's always safe to disable interrupts and clear the
	 * handlers, even if they have not been enabled/set. The same is true
	 * for disabling the interrupt subsystem.
	 */
	XUsbPs_Stop(UsbInstancePtr);
	XUsbPs_IntrDisable(UsbInstancePtr, XUSBPS_IXR_ALL);
	(int) XUsbPs_IntrSetHandler(UsbInstancePtr, NULL, NULL, 0);

	UsbDisableIntrSystem(IntcInstancePtr, UsbIntrId);

	/* Free allocated memory.
	 */
	if (NULL != UsbInstancePtr->UserDataPtr) {
		free(UsbInstancePtr->UserDataPtr);
	}
}
