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

// header file stuff

#endif

/*--------------------------------------------------------------*/
/* Functions Prototypes 										*/
/*--------------------------------------------------------------*/

int InitVPSS(unsigned int DeviceId);
int IicConfig(unsigned int DeviceIdPS);
int InitVDMA(unsigned int DeviceId);
int InitHdmi();
