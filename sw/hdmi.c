/*-----------------------------------------------------------------------------/
 /  !nso - HDMI		                                                           /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 / 	15/03/2023
 /	hdmi.c
 /
 /	Helper functions for setuping up HDMI
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Definition   				 								*/
/*--------------------------------------------------------------*/
#define DEBUG 1
#define VPSS_DEVICE_ID      XPAR_XV_CSC_0_DEVICE_ID
#define VPSS_INPUT_COLOUR   XVIDC_CSF_RGB
#define VPSS_OUTPUT_COLOUR  XVIDC_CSF_YCRCB_422
#define VPSS_FRAME_RATE     XVIDC_FR_60HZ
#define VPSS_SCREEN_HEIGHT  1080
#define VPSS_SCREEN_WIDTH   1920
#define VPSS_BYTES_PER_PIXEL 4
#define VPSS_INTERLACED     0
#define I2C_DEVICE_ID       XPAR_XIICPS_0_DEVICE_ID
#define VDMA_DEVICE_ID      XPAR_AXI_VDMA_0_DEVICE_ID
#define VDMA_BUFFER_0       0x03000000
#define VDMA_BUFFER_1       0x03800000
#define VDMA_BUFFER_2       0x04000000

//min size of buffer = 1920*1080*3 = 5EEC00

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#include "xil_printf.h"
#include "xvprocss.h"
#include "xil_exception.h"
#include "xiicps.h"
#include "xaxivdma.h"
#include "iic_utils.h"
#include "hdmi.h"
#include "xil_cache.h"

/*--------------------------------------------------------------*/
/* Global Variables				 								*/
/*--------------------------------------------------------------*/
extern XVprocSs VPSSInst;
extern XIicPs IicInst;
extern XAxiVdma VDMAInst;

int InitHdmi() {
	IicConfig(I2C_DEVICE_ID);
	InitVPSS(VPSS_DEVICE_ID);
	InitVDMA(VDMA_DEVICE_ID);

	// Set the iic mux to the ADV7511
	iic_write(&IicInst, 0x74, 0x2, 1);

	//Check the state of the HPD signal
	u8 monitor_connected = 0;
	u8 temp = 2;

	while (!monitor_connected) {
		monitor_connected = check_hdmi_hpd_status(&IicInst, 0x39);

		if (monitor_connected != temp) {
			temp = monitor_connected;
			if (monitor_connected) {
				xil_printf("HDMI Monitor connected\r\n");
			} else {
				xil_printf(
						"No HDMI Monitor connected / Monitor Disconnected\r\n");
			}
			sleep(2);
		}
	}

	// ADV7511 Basic Configuration
	configure_adv7511(&IicInst, 0x39);

	// ADV7511 Input / Output Mode
	//YCbCr 422 with Separated Syncs
	iic_write2(&IicInst, 0x39, 0x15, 0x1);
	//YCbCr444 Output Format, Style 1, 8bpc
	iic_write2(&IicInst, 0x39, 0x16, 0x38);
	//Video Input Justification: Right justified
	iic_write2(&IicInst, 0x39, 0x48, 0x08);

#if DEBUG == 1
	xil_printf("HDMI Setup Complete!\r\n");
#endif

	return XST_SUCCESS;
}

int InitVPSS(unsigned int DeviceId) {
	int status;

	//Configure the VPSS
	XVprocSs_Config *VprocCfgPtr;
	XVidC_VideoTiming const *TimingPtr;
	XVidC_VideoStream StreamIn, StreamOut;

	VprocCfgPtr = XVprocSs_LookupConfig(DeviceId);
	XVprocSs_CfgInitialize(&VPSSInst, VprocCfgPtr, VprocCfgPtr->BaseAddress);

	//Get the resolution details
	XVidC_VideoMode resId = XVidC_GetVideoModeId(VPSS_SCREEN_WIDTH,
			VPSS_SCREEN_HEIGHT, VPSS_FRAME_RATE, 0);
	TimingPtr = XVidC_GetTimingInfo(resId);

	//Set the input stream
	StreamIn.VmId = resId;
	StreamIn.Timing = *TimingPtr;
	StreamIn.ColorFormatId = VPSS_INPUT_COLOUR;
	StreamIn.ColorDepth = 8;
	StreamIn.PixPerClk = VprocCfgPtr->PixPerClock;
	StreamIn.FrameRate = VPSS_FRAME_RATE;
	StreamIn.IsInterlaced = VPSS_INTERLACED;
	XVprocSs_SetVidStreamIn(&VPSSInst, &StreamIn);

	//Set the output stream
	StreamOut.VmId = resId;
	StreamOut.Timing = *TimingPtr;
	StreamOut.ColorFormatId = VPSS_OUTPUT_COLOUR;
	StreamOut.ColorDepth = VprocCfgPtr->ColorDepth;
	StreamOut.PixPerClk = VprocCfgPtr->PixPerClock;
	StreamOut.FrameRate = VPSS_FRAME_RATE;
	StreamOut.IsInterlaced = VPSS_INTERLACED;
	XVprocSs_SetVidStreamOut(&VPSSInst, &StreamOut);

	status = XVprocSs_SetSubsystemConfig(&VPSSInst);
	if (status != XST_SUCCESS) {
		xil_printf("VPSS failed\r\n");
		return (XST_FAILURE);
	}

	//This is a very hacky way of doing this, i'm not sure why setting this reg to 0b1 or 0b11 or 0b10 flips the colours but it works.
	Xil_Out32(
			VPSSInst.Config.BaseAddress
					+ XV_CSC_CTRL_ADDR_HWREG_INVIDEOFORMAT_DATA, 0x01);

	#if DEBUG == 1
		xil_printf("VPSS Started.\r\n");
	#endif

	return XST_SUCCESS;
}

int IicConfig(unsigned int DeviceIdPS) {
	XIicPs_Config *Config;
	int status;

	/* Initialise the IIC driver so that it's ready to use */

	// Look up the configuration in the config table
	Config = XIicPs_LookupConfig(DeviceIdPS);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	// Initialise the IIC driver configuration
	status = XIicPs_CfgInitialize(&IicInst, Config, Config->BaseAddress);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	//Set the IIC serial clock rate.
	XIicPs_SetSClk(&IicInst, IIC_SCLK_RATE);

	#if DEBUG == 1
		xil_printf("IIC Setup Complete.\r\n");
	#endif

	return XST_SUCCESS;
}

int InitVDMA(unsigned int DeviceId) {

	int status;
	XAxiVdma_Config *config = XAxiVdma_LookupConfig(DeviceId);
	XAxiVdma_DmaSetup ReadCfg;
	status = XAxiVdma_CfgInitialize(&VDMAInst, config, config->BaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("DMA Initialization failed");
	}

	ReadCfg.VertSizeInput = VPSS_SCREEN_HEIGHT;
	ReadCfg.HoriSizeInput = VPSS_SCREEN_WIDTH * 4;
	ReadCfg.Stride = VPSS_SCREEN_WIDTH * 4;
	ReadCfg.FrameDelay = 0;
	ReadCfg.EnableCircularBuf = 0;
	ReadCfg.EnableSync = 0;
	ReadCfg.PointNum = 0;
	ReadCfg.EnableFrameCounter = 0;
	ReadCfg.FixedFrameStoreAddr = 0;
	status = XAxiVdma_DmaConfig(&VDMAInst, XAXIVDMA_READ, &ReadCfg);
	if (status != XST_SUCCESS) {
		xil_printf("Write channel config failed %d\r\n", status);
		return status;
	}

	ReadCfg.FrameStoreStartAddr[0] = VDMA_BUFFER_0;
	ReadCfg.FrameStoreStartAddr[1] = VDMA_BUFFER_1;
	ReadCfg.FrameStoreStartAddr[2] = VDMA_BUFFER_2;

	status = XAxiVdma_DmaSetBufferAddr(&VDMAInst, XAXIVDMA_READ,
			ReadCfg.FrameStoreStartAddr);
	if (status != XST_SUCCESS) {
		xil_printf("Read channel set buffer address failed %d\r\n", status);
		return XST_FAILURE;
	}

	Xil_DCacheFlush();

	status = XAxiVdma_DmaStart(&VDMAInst, XAXIVDMA_READ);
	if (status != XST_SUCCESS) {
		if (status == XST_VDMA_MISMATCH_ERROR)
			xil_printf("DMA Mismatch Error\r\n");
		return XST_FAILURE;
	}

	memset((int*)VDMA_BUFFER_0, 0xF, VPSS_SCREEN_HEIGHT*VPSS_SCREEN_WIDTH*VPSS_BYTES_PER_PIXEL);
	memset((int*)VDMA_BUFFER_1, 0xD18585, VPSS_SCREEN_HEIGHT*VPSS_SCREEN_WIDTH*VPSS_BYTES_PER_PIXEL);
	memset((int*)VDMA_BUFFER_2, 0x1A7211, VPSS_SCREEN_HEIGHT*VPSS_SCREEN_WIDTH*VPSS_BYTES_PER_PIXEL);

	#if DEBUG == 1
		xil_printf("VDMA Setup Complete.\r\n");
	#endif

	return XST_SUCCESS;
}

