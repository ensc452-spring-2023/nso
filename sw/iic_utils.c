// SPDX-License-Identifier: GPL-2.0
/*
 * iic_utils.c
 *
 *  Created on: 23 Nov 2018
 *      Author: Florent Werbrouck
 */

#include "iic_utils.h"

int ps_iic_init(u16 DeviceId, XIicPs* IicPs)
{
	int Status;
	XIicPs_Config *Config;

	Config = XIicPs_LookupConfig(DeviceId);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(IicPs, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Set the IIC serial clock rate.
	Status = XIicPs_SetSClk(IicPs, IIC_SCLK_RATE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

void set_iic_mux( XIicPs *IicPs, u8 MuxSelect, u8 Address)
{
	u8 data = MuxSelect;
	iic_write( IicPs, Address, data, 1 );
}

int iic_write( XIicPs *IicPs, u8 Address, u8 Data, s32 ByteCount)
{
	int Status;

	// Wait until bus is idle to start another transfer.
	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	// Send the buffer using the IIC and check for errors.
	Status = XIicPs_MasterSendPolled(IicPs, &Data, ByteCount, Address);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int iic_write2( XIicPs *IicPs, u8 Address, u8 Register, u8 Data )
{
	u8 WriteBuffer[2];
	int Status;

	/* A temporary write buffer must be used which contains both the address
	 * and the data to be written, put the address in first */
	WriteBuffer[0] = Register;
	WriteBuffer[1] = Data;


	// Wait until bus is idle to start another transfer.

	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	// Send the buffer using the IIC and check for errors.
	Status = XIicPs_MasterSendPolled(IicPs, WriteBuffer, 2, Address);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int iic_read( XIicPs *IicPs, u8 Address, u8 Register, u8 *Data, int ByteCount)
{

	int Status;

	// Wait until bus is idle to start another transfer.
	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	// Set the IIC Repeated Start option.
	Status = XIicPs_SetOptions(IicPs, XIICPS_REP_START_OPTION);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	// Set the address of the register to read
	Status = XIicPs_MasterSendPolled(IicPs, &Register, 1, Address);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Receive the data.
	Status = XIicPs_MasterRecvPolled(IicPs, Data, ByteCount, Address);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Clear the IIC Repeated Start option.
	Status = XIicPs_ClearOptions(IicPs, XIICPS_REP_START_OPTION);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int check_hdmi_hpd_status(XIicPs *IicPs, u8 Address)
{
	int Status;
	u8 data = 0x00;

	//
	Status = iic_read(IicPs, Address, ADV7511_HDP_REG_ADDR, &data, 1);
	// Status = iic_read2( IicPs, /*ZC702_HDMI_ADDR*/ 0x39, 0x42, &data, 1);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	if((data & ADV7511_HPD_CTRL_MASK) == ADV7511_HPD_CTRL_MASK) {
		// Monitor Connected
		return 1;
	}
	else
	{
		// Monitor not connected
		return 0;
	}
}

void configure_adv7511(XIicPs *IicPs, u8 Address)
{

	// Power-up the Tx
	iic_write2(IicPs, Address, 0x41,0x10);


	// Fixed registers that must be set on power up
	iic_write2(IicPs, Address, 0x98, 0x03);
	iic_write2(IicPs, Address, 0x9A, 0xE0);
	iic_write2(IicPs, Address, 0x9C, 0x30);
	iic_write2(IicPs, Address, 0x9D, 0x61);
	iic_write2(IicPs, Address, 0xA2, 0xA4);
	iic_write2(IicPs, Address, 0xA3, 0xA4);
	iic_write2(IicPs, Address, 0xE0, 0xD0);
	iic_write2(IicPs, Address, 0xF9, 0x00);

	// Color Space Conversion
	iic_write2(IicPs, Address, 0x18, 0xE7); //CSC Enabled (YCbCr to RGB)

	// Set RGB in AVinfo Frame
	iic_write2(IicPs, Address, 0x55, 0x00);
	// Aspect Ration
	iic_write2(IicPs, Address, 0x56, 0x28);

	// HPD Control always high
	iic_write2(IicPs, Address, 0xD6, 0xC0);

	// DVI Mode, no HDCP
	iic_write2(IicPs, Address, 0xAF, 0x04);

	// Fixed I2C Address
	iic_write2(IicPs, Address, 0xF9, 0x00);

}
