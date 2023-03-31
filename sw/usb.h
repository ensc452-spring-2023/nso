/*-----------------------------------------------------------------------------/
 /	!nso - Appended XUsbPs Host Mode										   /
 /-----------------------------------------------------------------------------/
 /	Bowie Gian
 /	04/03/2023
 /	usb.h
 /
 /	Referenced XUsbPs v2.6 Library
 /	I added the missing functions to use USB host mode with the XUsbPs library.
 /----------------------------------------------------------------------------*/

#ifndef USB_H
#define USB_H

/*--------------------------------------------------------------*/
/* Include Files												*/
/*--------------------------------------------------------------*/
#include "xusbps.h"
#include "xusbps_endpoint.h"
#include "xscugic.h"
#include <stdbool.h>

/*--------------------------------------------------------------*/
/* Definitions													*/
/*--------------------------------------------------------------*/
#define XUSBPS_REQ_GET_DESCRIPTOR	0x06
#define XUSBPS_REQ_SET_CONFIGURATION	0x09
#define POLLING_MS 8 // Polling rate in ms (1, 2, 4, 8)

#define MEMORY_SIZE (64 * 1024)

typedef u8	XUsbPs_pQH[XUSBPS_dQH_ALIGN];
typedef u8	XUsbPs_qTD[XUSBPS_dTD_ALIGN];

/*--------------------------------------------------------------*/
/* Structs														*/
/*--------------------------------------------------------------*/
/**
 * The following data structure represents a queue head.
 * This is based off the endpoints in XUsbPs.h
 */
typedef struct {
	XUsbPs_pQH *pQH;
	/**< Pointer to the Queue Head structure. */

	XUsbPs_qTD *qTDs;
	/**< List of pointers to the Transfer Descriptors of the
	 * endpoint. */

	XUsbPs_qTD *qTDHead;
	/**< Buffer to the next available descriptor in the list. */

	XUsbPs_qTD *qTDTail;
	/**< Buffer to the last unsent descriptor in the list*/

	u8 *qTDBufs;
	/**< Pointer to the first buffer of the buffer list for this
	 * endpoint. */

	XUsbPs_EpHandlerFunc	HandlerFunc;
	XUsbPs_EpIsoHandlerFunc HandlerIsoFunc;
	/**< Handler function for this endpoint. */
	void *HandlerRef;
	/**< User data reference for the handler. */
	u32	RequestedBytes;	/**< RequestedBytes for transfer */
	u32	BytesTxed;		/**< Actual Bytes transferred */
	u8 *BufferPtr;		/**< Buffer location */
	u32	Interval;		/**< Data transfer service interval */

	XUsbPs_pQH *QHHLP;
	/**< Pointer to the Queue Head Horizontal Link. */

	u32	Type;
	/**< Transaction Descriptor Type. Set to 01 (QH type) ~~Unused??~~ */

	u16	MaxPacketSize;
	/**< Maximum packet size for this Queue Head. This number will
	 * define the maximum number of bytes sent on the wire per
	 * transaction. Range: 0..1024 */

	u32 EpSpeed;
	/**<  Speed of the associated endpoint. (Pg. 471) */

	u32 EpNum;
	/**<  Endpoint number on the device. (Pg. 471) */

	u32 DeviceAddr;
	/**<  Address of device serving as the data source or sink. */

	u32	NumBufs;
	/**< Number of buffers to be handled by this Queue Head. */
	u32	BufSize;
	/**< Buffer size. */


} XUsbPs_QH;

/**
 * The XUsbPs_HostConfig structure contains the configuration information to
 * configure the USB controller for HOST mode. This data structure is used
 * with the XUsbPs_ConfigureHost() function call.
 */
typedef struct {
	u8  NumQHs;	/**< Number of Queue Heads for the controller. */

	u32 DMAMemPhys;		/**< Physical base address of DMAable memory
				  allocated for the driver. */

				  /* The following members are used internally by the L0/L1 driver.  They
				   * MUST NOT be accesses and/or modified in any way by the upper layers.
				   *
				   * The reason for having these members is that we generally try to
				   * avoid allocating memory in the L0/L1 driver as we want to be OS
				   * independent. In order to avoid allocating memory for this data
				   * structure wihin L0/L1 we put it into the XUsbPs_DeviceConfig
				   * structure which is allocated by the caller.
				   */
	XUsbPs_QH	QueueHead[XUSBPS_MAX_ENDPOINTS];
	/**< List of queue head metadata structures. TEMP USING EP LENGTH*/

	u32 PhysAligned;	/**< 64 byte aligned base address of the DMA
				   memory block. Will be computed and set by
				   the L0/L1 driver. */
} XUsbPs_HostConfig;

typedef struct {
	XUsbPs usbInstance;
	XUsbPs_HostConfig HostConfig;
} UsbWithHost;

bool USB_SetupDevice(UsbWithHost *usbWithHostInstancePtr, int status);
XUsbPs_qTD *USB_SetupPolling(UsbWithHost *usbWithHostInstancePtr);
XUsbPs_qTD* USB_qTDActivateIn(XUsbPs_QH *QueueHead, bool isIOC, bool dataNum);

// from xusbps_intr_example.c
// https://github.com/Xilinx/embeddedsw/tree/master/XilinxProcessorIPLib/drivers/usbps/examples
int UsbSetupIntrSystem(XScuGic *IntcInstancePtr, XUsbPs *UsbInstancePtr, u16 UsbIntrId);
void UsbDisableIntrSystem(XScuGic *IntcInstancePtr, u16 UsbIntrId);
int USB_Setup(XScuGic *IntcInstancePtr, UsbWithHost *usbWithHostInstancePtr, u16 UsbDeviceId, u16 UsbIntrId, void *UsbIntrHandler);
void USB_CleanUp(XScuGic *IntcInstancePtr, XUsbPs *UsbInstancePtr, u16 UsbIntrId);

/**
 * Queue Transfer Descriptor
 *
 * The qTD describes to the host controller the USB token, location
 * and quantity of data to be sent/received for given transfer.
 */
#define XUSBPS_qTDNLP	0x00 /**< Pointer to the next descriptor */
#define XUSBPS_qTDANLP	0x04 /**< Pointer to the alternate next descriptor */
#define XUSBPS_qTDTOKEN	0x08 /**< Descriptor Token */
#define XUSBPS_qTDBPTR0	0x0C /**< Buffer Pointer 0 */
#define XUSBPS_qTDBPTR1	0x10 /**< Buffer Pointer 1 */
#define XUSBPS_qTDBPTR2	0x14 /**< Buffer Pointer 2 */
#define XUSBPS_qTDBPTR3	0x18 /**< Buffer Pointer 3 */
#define XUSBPS_qTDBPTR4	0x1C /**< Buffer Pointer 4 */
#define XUSBPS_qTDBPTR(n)	(XUSBPS_qTDBPTR0 + (n) * 0x04)


/** @name qTD Next Link Pointer (qTDNLP/qTDANLP) bit positions.
 *  @{
 */
#define XUSBPS_qTDNLP_T_MASK		0x00000001
				/**< USB qTD Next Link Pointer Terminate Bit */
#define XUSBPS_qTDNLP_ADDR_MASK	0xFFFFFFE0
				/**< USB qTD Next Link Pointer Address [31:5] */
/* @} */


/** @name qTD Token (qTDTOKEN) bit positions.
 *  @{
 */
#define XUSBPS_qTDTOKEN_PING_MASK	0x00000001 /**< qTD Ping State (0:OUT, 1:Ping) or ERR Status */
#define XUSBPS_qTDTOKEN_STRANS_MASK	0x00000002 /**< qTD Split Transaction State Status (0:Start Split, 1:Start Split) */
#define XUSBPS_qTDTOKEN_MFRAME_MASK	0x00000004 /**< qTD Missed Microframe Status. */
#define XUSBPS_qTDTOKEN_XERR_MASK	0x00000008 /**< qTD Transaction Error */
#define XUSBPS_qTDTOKEN_BABBLE_MASK	0x00000010 /**< qTD Babble Detected Status */
#define XUSBPS_qTDTOKEN_BUFERR_MASK	0x00000020 /**< qTD Data Buffer Error */
#define XUSBPS_qTDTOKEN_HALT_MASK	0x00000040 /**< qTD Halted Flag */
#define XUSBPS_qTDTOKEN_ACTIVE_MASK	0x00000080 /**< qTD Active Bit */
#define XUSBPS_qTDTOKEN_PID_MASK	0x00000300 /**< qTD Packet ID Code [1:0] */
#define XUSBPS_qTDTOKEN_PID_OUT		0x00000000 /**< host-to-function */
#define XUSBPS_qTDTOKEN_PID_IN		0x00000100 /**< function-to-host */
#define XUSBPS_qTDTOKEN_PID_SETUP	0x00000200 /**< host-to-function for SETUP */
#define XUSBPS_qTDTOKEN_CERR_MASK	0x00000C00 /**< Error Counter, Cerr [1:0] */
#define XUSBPS_qTDTOKEN_CPAGE_MASK	0x00007000 /**< Current Page, C_Page [2:0] */
#define XUSBPS_qTDTOKEN_IOC_MASK	0x00008000 /**< Interrupt on Complete Bit */
#define XUSBPS_qTDTOKEN_LEN_MASK	0x7FFF0000 /**< Transfer Length Field */
#define XUSBPS_qTDTOKEN_DT_MASK		0x80000000 /**< Data Toggle, DT */
/* @} */


/**
 * Endpoint Queue Head (MODIFIED FROM DEVICE)
 *
 * Device queue heads are arranged in an array in a continuous area of memory
 * pointed to by the ENDPOINTLISTADDR pointer. The device controller will index
 * into this array based upon the endpoint number received from the USB bus.
 * All information necessary to respond to transactions for all primed
 * transfers is contained in this list so the Device Controller can readily
 * respond to incoming requests without having to traverse a linked list.
 *
 * The device Endpoint Queue Head (dQH) is where all transfers are managed. The
 * dQH is a 48-byte data structure, but must be aligned on a 64-byte boundary.
 * During priming of an endpoint, the dTD (device transfer descriptor) is
 * copied into the overlay area of the dQH, which starts at the nextTD pointer
 * DWord and continues through the end of the buffer pointers DWords. After a
 * transfer is complete, the dTD status DWord is updated in the dTD pointed to
 * by the currentTD pointer. While a packet is in progress, the overlay area of
 * the dQH is used as a staging area for the dTD so that the Device Controller
 * can access needed information with little minimal latency.
 *
 * @note
 *    Software must ensure that no interface data structure reachable by the
 *    Device Controller spans a 4K-page boundary.  The first element of the
 *    Endpoint Queue Head List must be aligned on a 4K boundary.
 */
#define XUSBPS_QHHLPTR			0x00 /**< QH Horizontal Link Pointer */
#define XUSBPS_QHCFG0			0x04 /**< QH Configuration 0 */
#define XUSBPS_QHCFG1			0x08 /**< QH Configuration 1 */
#define XUSBPS_QHCPTR			0x0C /**< QH Current dTD Pointer */
#define XUSBPS_QHqTDNLP			0x10 /**< qTD Next Link Ptr in QH overlay */
#define XUSBPS_QHqTDANLP		0x14 /**< qTD Alt Next Link Ptr in QH overlay */
#define XUSBPS_QHqTDTOKEN		0x18 /**< qTD Token in QH overlay */


/** @name QH Configuration (QHCFG) bit positions.
 *  @{
 */
#define XUSBPS_QHCFG0_DADDR_MASK	0x0000007F
					/**< USB QH Device Address [6:0] */
#define XUSBPS_QHCFG0_MPL_MASK		0x07FF0000
					/**< USB QH Maximum Packet Length Field [10:0] */
#define XUSBPS_QHCFG0_MPL_SHIFT    16

#define XUSBPS_QHCFG1_MULT_MASK		0xC0000000
					/* USB QH Number of Transactions Field [1:0] */
#define XUSBPS_QHCFG1_MULT_SHIFT  30
/* @} */


/*****************************************************************************/
/**
 *
 * This macro gets the Next Link pointer of the given Transfer Descriptor.
 *
 * @param	qTDPtr is pointer to the qTD element.
 *
 * @return 	Next Link pointer of the descriptor.
 *
 * @note	C-style signature:
 *		u32 XUsbPs_qTDGetNLP(u32 qTDPtr)
 *
 ******************************************************************************/
#define XUsbPs_qTDGetNLP(qTDPtr)					\
		(XUsbPs_dTD *) ((XUsbPs_ReaddTD(dTDPtr, XUSBPS_dTDNLP)\
					& XUSBPS_qTDNLP_ADDR_MASK))

/*****************************************************************************/
/**
 *
 * This macro checks if the given descriptor is active.
 *
 * @param	qTDPtr is a pointer to the qTD element.
 *
 * @return
 * 		- TRUE: The buffer is active.
 * 		- FALSE: The buffer is not active.
 *
 * @note	C-style signature:
 *		int XUsbPs_qTDIsActive(u32 qTDPtr)
 *
 ******************************************************************************/
#define XUsbPs_qTDIsActive(qTDPtr)					\
		((XUsbPs_ReaddTD(qTDPtr, XUSBPS_qTDTOKEN) &		\
				XUSBPS_dTDTOKEN_ACTIVE_MASK) ? TRUE : FALSE)


/*****************************************************************************/
/**
 *
 * This macro sets the Transfer Length for the given Transfer Descriptor.
 *
 * @param	qTDPtr is pointer to the qTD element.
 * @param	Len is the length to be set. Range: 0..16384
 *
 * @note	C-style signature:
 *		void XUsbPs_qTDSetTransferLen(u32 qTDPtr, u32 Len)
 *
 ******************************************************************************/
#define XUsbPs_qTDSetTransferLen(qTDPtr, Len)				\
		XUsbPs_WritedTD(qTDPtr, XUSBPS_qTDTOKEN, 		\
			(XUsbPs_ReaddTD(qTDPtr, XUSBPS_qTDTOKEN) &	\
				~XUSBPS_qTDTOKEN_LEN_MASK) | ((Len) << 16))

#endif
