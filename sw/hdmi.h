/*-----------------------------------------------------------------------------/
 /  !nso - HDMI		                                                           /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 / 	15/03/2023
 /	hdmi.h
 /
 /	Helper functions for setuping up HDMI
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#ifndef HDMI_H
#define HDMI_H

/*--------------------------------------------------------------*/
/* Definition   				 								*/
/*--------------------------------------------------------------*/
#define VPSS_DEVICE_ID      XPAR_XV_CSC_0_DEVICE_ID
#define VPSS_INPUT_COLOUR   XVIDC_CSF_RGB
#define VPSS_OUTPUT_COLOUR  XVIDC_CSF_YCRCB_422
#define VPSS_FRAME_RATE     XVIDC_FR_60HZ
#define VPSS_SCREEN_HEIGHT  1080
#define VPSS_SCREEN_WIDTH   1920
#define VPSS_BYTES_PER_PIXEL 4
#define VPSS_INTERLACED     0
#define I2C_DEVICE_ID       XPAR_XIICPS_1_DEVICE_ID
#define VDMA_DEVICE_ID      XPAR_AXI_VDMA_0_DEVICE_ID
#define VDMA_BUFFER_0       0x08000000
#define VDMA_BUFFER_1       0x08800000
#define VDMA_BUFFER_2       0x09000000

#endif

/*--------------------------------------------------------------*/
/* Definition   				 								*/
/*--------------------------------------------------------------*/
#define VPSS_DEVICE_ID      XPAR_XV_CSC_0_DEVICE_ID
#define VPSS_INPUT_COLOUR   XVIDC_CSF_RGB
#define VPSS_OUTPUT_COLOUR  XVIDC_CSF_YCRCB_422
#define VPSS_FRAME_RATE     XVIDC_FR_60HZ
#define VPSS_SCREEN_HEIGHT  1080
#define VPSS_SCREEN_WIDTH   1920
#define VPSS_BYTES_PER_PIXEL 4
#define VPSS_INTERLACED     0
#define HDMI_I2C_DEVICE_ID  XPAR_XIICPS_1_DEVICE_ID
#define VDMA_DEVICE_ID      XPAR_AXI_VDMA_0_DEVICE_ID
#define VDMA_BUFFER_0       0x08000000
#define VDMA_BUFFER_1       0x08800000
#define VDMA_BUFFER_2       0x09000000

/*--------------------------------------------------------------*/
/* Functions Prototypes 										*/
/*--------------------------------------------------------------*/

int InitVPSS(unsigned int DeviceId);
int IicConfig(unsigned int DeviceIdPS);
int InitVDMA(unsigned int DeviceId);
int InitHdmi();
