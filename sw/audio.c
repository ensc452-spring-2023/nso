/*
 * audio.c
 *
 *  Contains all of the functions related to audio codec setup.
 */
#include "audio.h"

/* ---------------------------------------------------------------------------- *
 * 								Header Files									*
 * ---------------------------------------------------------------------------- */
#include <stdio.h>
#include <xil_io.h>
#include <sleep.h>
#include "xiicps.h"
#include <xil_printf.h>
#include <xparameters.h>
#include "xgpio.h"
#include "xuartps.h"
#include "stdlib.h"
#include "xaxidma.h"
#include "xil_exception.h"

/* ---------------------------------------------------------------------------- *
 * 							Definitions								        	*
 * ---------------------------------------------------------------------------- */

#define	AUDIO_DMA_BUFFERSIZE 4000
#define AUDIO_DMA_DEVICE_ID		XPAR_AXI_DMA_0_DEVICE_ID

/* ---------------------------------------------------------------------------- *
 * 							Global Variables									*
 * ---------------------------------------------------------------------------- */
XIicPs Iic;
XAxiDma AudioDMA;
XAxiDma_Bd AudioDMABdBuffer[AUDIO_DMA_BUFFERSIZE]__attribute__((aligned(XAXIDMA_BD_MINIMUM_ALIGNMENT)));
int audioDMAWritten;

/* ---------------------------------------------------------------------------- *
 * 									IicConfig()									*
 * ---------------------------------------------------------------------------- *
 * Initialises the IIC driver by looking up the configuration in the config
 * table and then initialising it. Also sets the IIC serial clock rate.
 * ---------------------------------------------------------------------------- */
int AudioDMASetup() {
	int Status = XAxiDma_CfgInitialize(&AudioDMA, XAxiDma_LookupConfig(AUDIO_DMA_DEVICE_ID));
	if(Status != XST_SUCCESS){
		xil_printf("Failled to initalize DMA\r\n");
	}

	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AudioDMA);

	audioDMAWritten = FALSE;

	// Disable all TX interrupts before TxBD space setup
	XAxiDma_BdRingIntDisable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);

	// Setup TxBD space
	u32 BdCount = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT,
			(u32 ) sizeof(AudioDMABdBuffer));

	Status = XAxiDma_BdRingCreate(TxRingPtr, (UINTPTR) &AudioDMABdBuffer[0],
			(UINTPTR) &AudioDMABdBuffer[0], XAXIDMA_BD_MINIMUM_ALIGNMENT,
			BdCount);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed create BD ring\r\n");
	}

	// Like the RxBD space, we create a template and set all BDs to be the
	// same as the template. The sender has to set up the BDs as needed.
	XAxiDma_Bd BdTemplate;
	XAxiDma_BdClear(&BdTemplate);
	Status = XAxiDma_BdRingClone(TxRingPtr, &BdTemplate);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed clone BDs\r\n");
	}

	// Start the TX channel
	Status = XAxiDma_BdRingStart(TxRingPtr);
	//Status = XAxiDma_StartBdRingHw(TxRingPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed bd start\r\n");
	}

	return 0;
}

int AudioDMAIsBusy() {
	if (audioDMAWritten && XAxiDma_Busy(&AudioDMA, XAXIDMA_DMA_TO_DEVICE) != 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void AudioDMAReset() {
	XAxiDma_Reset(&AudioDMA);
	for (int TimeOut = 2000000; TimeOut > 0; --TimeOut) {
		if (XAxiDma_ResetIsDone(&AudioDMA)) {
			break;
		}
	}
}

void AudioDMAFreeProcessedBDs() {
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AudioDMA);

	// Get all processed BDs from hardware
	XAxiDma_Bd *BdPtr;
	int BdCount = XAxiDma_BdRingFromHw(TxRingPtr, XAXIDMA_ALL_BDS, &BdPtr);

	// Free all processed BDs for future transmission
	int Status = XAxiDma_BdRingFree(TxRingPtr, BdCount, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to free BDs\r\n");
	}
}

// These blocks can have a maximum size of "TxRingPtr->MaxTransferLen" (around 8 MBytes)
void AudioDMATransmit(u32*buffer, int buffer_len, int nRepeats) {
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AudioDMA);

	// Free the processed BDs from previous run.
	AudioDMAFreeProcessedBDs();

	// Flush the SrcBuffer before the DMA transfer, in case the Data Cache is enabled
	Xil_DCacheFlushRange((u32) buffer, buffer_len * sizeof(u32));

	XAxiDma_Bd *BdPtr = NULL;
	int Status = XAxiDma_BdRingAlloc(TxRingPtr, nRepeats, &BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed bd alloc\r\n");
	}

	XAxiDma_Bd *BdCurPtr = BdPtr;
	;
	for (int i = 0; i < nRepeats; ++i) {
		Status = XAxiDma_BdSetBufAddr(BdCurPtr, (UINTPTR) buffer);
		if (Status != XST_SUCCESS) {
			xil_printf("Tx set buffer addr failed\r\n");
		}

		Status = XAxiDma_BdSetLength(BdCurPtr, buffer_len,
				TxRingPtr->MaxTransferLen);
		if (Status != XST_SUCCESS) {
			xil_printf("Tx set length failed\r\n");
		}

		u32 CrBits = 0;
		if (i == 0) {
			CrBits |= XAXIDMA_BD_CTRL_TXSOF_MASK; // First BD
		}
		if (i == nRepeats - 1) {
			CrBits |= XAXIDMA_BD_CTRL_TXEOF_MASK; // Last BD
		}
		XAxiDma_BdSetCtrl(BdCurPtr, CrBits);

		XAxiDma_BdSetId(BdCurPtr, (UINTPTR )buffer);

		BdCurPtr = (XAxiDma_Bd *) XAxiDma_BdRingNext(TxRingPtr, BdCurPtr);
	}

	// Give the BD to hardware
	Status = XAxiDma_BdRingToHw(TxRingPtr, nRepeats, BdPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to hw\r\n");
	}

	audioDMAWritten = TRUE;
}

void AudioDMATransmitSong(u32*buffer, int buffer_len)
{
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AudioDMA);

	u32 nSamplesRemain = buffer_len;
	u32 maxBlockSize = TxRingPtr->MaxTransferLen/4;
	u32 * pBlock = (u32*) buffer;

	while(nSamplesRemain > 0){
		u32 nTransfer = nSamplesRemain > maxBlockSize ? maxBlockSize : nSamplesRemain;
		AudioDMATransmit(pBlock, nTransfer, 1);
		nSamplesRemain -= nTransfer;
		pBlock += nTransfer/4;
	}
}

void InitAudio() {
	AudioPllConfig();
	AudioConfigureJacks();
	LineinLineoutConfig();
	AudioDMASetup();

}

unsigned char Iic1Config(unsigned int DeviceIdPS) {
	XIicPs_Config *Config;
	int Status;

	/* Initialise the IIC driver so that it's ready to use */

	// Look up the configuration in the config table
	Config = XIicPs_LookupConfig(DeviceIdPS);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	// Initialise the IIC driver configuration
	Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	//Set the IIC serial clock rate.
	XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);

	return XST_SUCCESS;
}

/* ---------------------------------------------------------------------------- *
 * 								AudioPllConfig()								*
 * ---------------------------------------------------------------------------- *
 * Configures audio codes's internal PLL. With MCLK = 10 MHz it configures the
 * PLL for a VCO frequency = 49.152 MHz, and an audio sample rate of 48 KHz.
 * ---------------------------------------------------------------------------- */
void AudioPllConfig() {

	unsigned char u8TxData[8], u8RxData[6];
	int Status;

	Status = Iic1Config(AUDIO_I2C_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("\nError initializing IIC");

	}

	// Disable Core Clock
	AudioWriteToReg(R0_CLOCK_CONTROL, 0x0E);

	/* 	MCLK = 10 MHz
	 R = 0100 = 4, N = 0x02 0x3C = 572, M = 0x02 0x71 = 625

	 PLL required output = 1024x48 KHz
	 (PLLout)			= 49.152 MHz

	 PLLout/MCLK			= 49.152 MHz/10 MHz
	 = 4.9152 MHz
	 = R + (N/M)
	 = 4 + (572/625) */

	// Write 6 bytes to R1 @ register address 0x4002
	u8TxData[0] = 0x40; // Register write address [15:8]
	u8TxData[1] = 0x02; // Register write address [7:0]
	u8TxData[2] = 0x03; // byte 6 - M[15:8]
	u8TxData[3] = 0xE8; // byte 5 - M[7:0]
	u8TxData[4] = 0x03; // byte 4 - N[15:8]
	u8TxData[5] = 0xA4; // byte 3 - N[7:0]
	u8TxData[6] = 0x19; // byte 2 - 7 = reserved, bits 6:3 = R[3:0], 2:1 = X[1:0], 0 = PLL operation mode
	u8TxData[7] = 0x01; // byte 1 - 7:2 = reserved, 1 = PLL Lock, 0 = Core clock enable

	// Write bytes to PLL Control register R1 @ 0x4002
	XIicPs_MasterSendPolled(&Iic, u8TxData, 8, (IIC_SLAVE_ADDR >> 1));
	while (XIicPs_BusIsBusy(&Iic))
		;

	// Register address set: 0x4002
	u8TxData[0] = 0x40;
	u8TxData[1] = 0x02;

	// Poll PLL Lock bit
	do {
		XIicPs_MasterSendPolled(&Iic, u8TxData, 2, (IIC_SLAVE_ADDR >> 1));
		while (XIicPs_BusIsBusy(&Iic))
			;
		XIicPs_MasterRecvPolled(&Iic, u8RxData, 6, (IIC_SLAVE_ADDR >> 1));
		while (XIicPs_BusIsBusy(&Iic))
			;
	} while ((u8RxData[5] & 0x02) == 0); // while not locked

	AudioWriteToReg(R0_CLOCK_CONTROL, 0x0F);	// 1111
												// bit 3:		CLKSRC = PLL Clock input
												// bits 2:1:	INFREQ = 1024 x fs
												// bit 0:		COREN = Core Clock enabled

	AudioWriteToReg(R15_SERIAL_PORT_CONTROL_0, 0x01);
}

/* ---------------------------------------------------------------------------- *
 * 								AudioWriteToReg									*
 * ---------------------------------------------------------------------------- *
 * Function to write one byte (8-bits) to one of the registers from the audio
 * controller.
 * ---------------------------------------------------------------------------- */
void AudioWriteToReg(unsigned char u8RegAddr, unsigned char u8Data) {

	unsigned char u8TxData[3];

	u8TxData[0] = 0x40;
	u8TxData[1] = u8RegAddr;
	u8TxData[2] = u8Data;

	XIicPs_MasterSendPolled(&Iic, u8TxData, 3, (IIC_SLAVE_ADDR >> 1));
	while (XIicPs_BusIsBusy(&Iic))
		;
}

/* ---------------------------------------------------------------------------- *
 * 								AudioConfigureJacks()							*
 * ---------------------------------------------------------------------------- *
 * Configures audio codes's various mixers, ADC's, DAC's, and amplifiers to
 * accept stereo input from line in and push stereo output to line out.
 * ---------------------------------------------------------------------------- */
void AudioConfigureJacks() {
	AudioWriteToReg(R4_RECORD_MIXER_LEFT_CONTROL_0, 0x01); //enable mixer 1
	AudioWriteToReg(R5_RECORD_MIXER_LEFT_CONTROL_1, 0x07); //unmute Left channel of line in into mxr 1 and set gain to 6 db
	AudioWriteToReg(R6_RECORD_MIXER_RIGHT_CONTROL_0, 0x01); //enable mixer 2
	AudioWriteToReg(R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x07); //unmute Right channel of line in into mxr 2 and set gain to 6 db
	AudioWriteToReg(R19_ADC_CONTROL, 0x13); //enable ADCs

	AudioWriteToReg(R22_PLAYBACK_MIXER_LEFT_CONTROL_0, 0x21); //unmute Left DAC into Mxr 3; enable mxr 3
	AudioWriteToReg(R24_PLAYBACK_MIXER_RIGHT_CONTROL_0, 0x41); //unmute Right DAC into Mxr4; enable mxr 4
	AudioWriteToReg(R26_PLAYBACK_LR_MIXER_LEFT_LINE_OUTPUT_CONTROL, 0x05); //unmute Mxr3 into Mxr5 and set gain to 6db; enable mxr 5
	AudioWriteToReg(R27_PLAYBACK_LR_MIXER_RIGHT_LINE_OUTPUT_CONTROL, 0x11); //unmute Mxr4 into Mxr6 and set gain to 6db; enable mxr 6
	AudioWriteToReg(R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL, 0xFF); //Mute Left channel of HP port (LHP)
	AudioWriteToReg(R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL, 0xFF); //Mute Right channel of HP port (LHP)
	//AudioWriteToReg(R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xE6); //set LOUT volume (0db); unmute left channel of Line out port; set Line out port to line out mode
	//AudioWriteToReg(R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xE6); // set ROUT volume (0db); unmute right channel of Line out port; set Line out port to line out mode
	AudioWriteToReg(R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xFE); //set LOUT volume (0db); unmute left channel of Line out port; set Line out port to line out mode
	AudioWriteToReg(R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xFE); // set ROUT volume (0db); unmute right channel of Line out port; set Line out port to line out mode
	AudioWriteToReg(R35_PLAYBACK_POWER_MANAGEMENT, 0x03); //enable left and right channel playback (not sure exactly what this does...)
	AudioWriteToReg(R36_DAC_CONTROL_0, 0x03); //enable both DACs

	AudioWriteToReg(R58_SERIAL_INPUT_ROUTE_CONTROL, 0x01); //Connect I2S serial port output (SDATA_O) to DACs
	AudioWriteToReg(R59_SERIAL_OUTPUT_ROUTE_CONTROL, 0x01); //connect I2S serial port input (SDATA_I) to ADCs

	AudioWriteToReg(R65_CLOCK_ENABLE_0, 0x7F); //Enable clocks
	AudioWriteToReg(R66_CLOCK_ENABLE_1, 0x03); //Enable rest of clocks
}

/* ---------------------------------------------------------------------------- *
 * 								LineinLineoutConfig()							*
 * ---------------------------------------------------------------------------- *
 * Configures Line-In input, ADC's, DAC's, Line-Out and HP-Out.
 * ---------------------------------------------------------------------------- */
void LineinLineoutConfig() {

	AudioWriteToReg(R17_CONVERTER_CONTROL_0, 0x00); //48 KHz
	AudioWriteToReg(R64_SERIAL_PORT_SAMPLING_RATE, 0x00); //48 KHz 0101 0110
	AudioWriteToReg(R19_ADC_CONTROL, 0x13);
	AudioWriteToReg(R36_DAC_CONTROL_0, 0x03);
	AudioWriteToReg(R35_PLAYBACK_POWER_MANAGEMENT, 0x03);
	AudioWriteToReg(R58_SERIAL_INPUT_ROUTE_CONTROL, 0x01);
	AudioWriteToReg(R59_SERIAL_OUTPUT_ROUTE_CONTROL, 0x01);
	AudioWriteToReg(R65_CLOCK_ENABLE_0, 0x7F);
	AudioWriteToReg(R66_CLOCK_ENABLE_1, 0x03);

	AudioWriteToReg(R4_RECORD_MIXER_LEFT_CONTROL_0, 0x01);
	AudioWriteToReg(R5_RECORD_MIXER_LEFT_CONTROL_1, 0x05); //0 dB gain
	AudioWriteToReg(R6_RECORD_MIXER_RIGHT_CONTROL_0, 0x01);
	AudioWriteToReg(R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x05); //0 dB gain

	AudioWriteToReg(R22_PLAYBACK_MIXER_LEFT_CONTROL_0, 0x21);
	AudioWriteToReg(R24_PLAYBACK_MIXER_RIGHT_CONTROL_0, 0x41);
	AudioWriteToReg(R26_PLAYBACK_LR_MIXER_LEFT_LINE_OUTPUT_CONTROL, 0x03); //0 dB
	AudioWriteToReg(R27_PLAYBACK_LR_MIXER_RIGHT_LINE_OUTPUT_CONTROL, 0x09); //0 dB
	AudioWriteToReg(R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL, 0xE7); //0 dB
	AudioWriteToReg(R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL, 0xE7); //0 dB
	AudioWriteToReg(R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xE6); //0 dB
	AudioWriteToReg(R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xE6); //0 dB
}
