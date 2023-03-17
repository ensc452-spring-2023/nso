// Modified from Zynq Book interrupt_counter_tut_2B.c
#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include "xttcps.h"
#include "xvprocss.h"
#include "xiicps.h"
#include "xaxivdma.h"

#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"
#include "xil_cache.h"
#include "xil_mmu.h" // For Xil_SetTlbAttributes();

#include "xusbps.h"			/* USB controller driver */
#include "xusbps_endpoint.h"
#include "xusbps_hw.h"
#include "xusbps_ch9.h"		/* Generic Chapter 9 handling code */
#include "xusbps_class_storage.h"	/* Storage class handling code */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "usb.h"
#include "graphics.h"

#include "game_menu.h"
#include "game_logic.h"
#include "graphics.h"
#include "sd.h"
#include "hdmi.h"

// Parameter definitions
#define INTC_DEVICE_ID 			XPAR_PS7_SCUGIC_0_DEVICE_ID
#define BTNS_DEVICE_ID			XPAR_AXI_GPIO_0_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID 	XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define TIMER_DEVICE_ID 		XPAR_PS7_TTC_0_DEVICE_ID
#define BTN_INT 				XGPIO_IR_CH1_MASK
#define TIMER_IRPT_INTR 		XPS_TTC0_0_INT_ID
#define VBLANK_INTR				XPAR_FABRIC_INTERRUPT_CONVERTER_0_IRQ_OUT_INTR

XGpio BTNInst;
XScuGic INTCInst;
FATFS FS_instance;
XTtcPs Timer; //timer
XVprocSs VPSSInst;
XIicPs IicInst;
XAxiVdma VDMAInst;
static XUsbPs UsbInstance; /* The instance of the USB Controller */

static int btn_value;
HitObject * gameHitobjects;
int numberOfHitobjects = 0;
u32 bgColour = 0x1F1F1F;

// Define button values
#define BTN_CENTER 	1
#define BTN_DOWN 	2
#define BTN_LEFT 	4
#define BTN_RIGHT 	8
#define BTN_UP 		16

#define WILLIAMMOUSE 1

typedef struct {
	u32 OutputHz; /* Output frequency */
	u16 Interval; /* Interval value */
	u8 Prescaler; /* Prescaler value */
	u16 Options; /* Option settings */
} TmrCntrSetup;

extern int *image_output_buffer;

// Sprites
int *imageMenu;
int *imageBg;
int *imageCircle;
int *imageCircleOverlay;
int *spinner;
int *imageRanking;
int *imageNum[10];

long time = 0;

//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
static void BTN_Intr_Handler(void *baseaddr_p);
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr);
static void TimerIntrHandler(void *CallBackRef);
void VBlankIntrHandler(void *InstancePtr);

static unsigned int score = 0;

#define SCREEN_MENU 0
#define SCREEN_GAME 1
#define SCREEN_STAT 2
static int screen = SCREEN_MENU;

void initTimer() {
	XTtcPs_Config *TMRConfigPtr; //timer config

	TMRConfigPtr = XTtcPs_LookupConfig(TIMER_DEVICE_ID);
	XTtcPs_CfgInitialize(&Timer, TMRConfigPtr, TMRConfigPtr->BaseAddress);
	XTtcPs_SelfTest(&Timer);

	TmrCntrSetup TimerSetup;

	TimerSetup.Options |= (XTTCPS_OPTION_INTERVAL_MODE
			| XTTCPS_OPTION_WAVE_DISABLE);
	TimerSetup.Interval = 0;
	TimerSetup.Prescaler = 0;
	TimerSetup.OutputHz = 333;

	XTtcPs_SetOptions(&Timer, TimerSetup.Options);
	XTtcPs_CalcIntervalFromFreq(&Timer, TimerSetup.OutputHz,
			&(TimerSetup.Interval), &(TimerSetup.Prescaler));
	XTtcPs_SetInterval(&Timer, TimerSetup.Interval);
	XTtcPs_SetPrescaler(&Timer, TimerSetup.Prescaler);
}

//----------------------------------------------------
// INTERRUPT HANDLER FUNCTIONS
// - called by the timer, button interrupt
// - modified to change VGA output
//----------------------------------------------------
void BTN_Intr_Handler(void *InstancePtr) {
	// Disable GPIO interrupts
	XGpio_InterruptDisable(&BTNInst, BTN_INT);

	// Ignore additional button presses
	if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) != BTN_INT) {
		return;
	}

	btn_value = XGpio_DiscreteRead(&BTNInst, 1);

	// Change display when buttons are pressed
	if (btn_value == BTN_CENTER) {
		score = 0;
		screen = SCREEN_MENU;
		DrawMenu();
	} else if (btn_value == BTN_LEFT) {
		screen = SCREEN_GAME;
		DrawGame(score);
	} else if (btn_value == BTN_RIGHT) {
		screen = SCREEN_STAT;
		DrawStats(score);
	} else if (btn_value == BTN_UP) {
		score += 369;
		if (screen == SCREEN_GAME)
			DrawGame(score);
		else if (screen == SCREEN_STAT)
			DrawStats(score);
	} else if (btn_value == BTN_DOWN) {
		score -= 144;
		if (screen == SCREEN_GAME)
			DrawGame(score);
		else if (screen == SCREEN_STAT)
			DrawStats(score);
	}

	(void) XGpio_InterruptClear(&BTNInst, BTN_INT);

	// Enable GPIO interrupts
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
}

int activeFrameBuffer = 0;
int drawingFrameBuffer = 1;
int * frameBuffers[3] = {0x02000000, 0x02800000, 0x03000000};

static void TimerIntrHandler(void *CallBackRef) {
	XTtcPs_GetInterruptStatus((XTtcPs * ) CallBackRef);
	XTtcPs_ClearInterruptStatus((XTtcPs * ) CallBackRef, Interrupt_staus);

	time++;


}

int status = 0;
bool isSetupComplete = false;
bool isPolling = false;
XUsbPs_qTD *qTDPollReceiver = 0;

#define MOUSE_LMB_MASK 0x01
#define MOUSE_RMB_MASK 0x02

static void UsbIntrHandler(void *CallBackRef, u32 Mask) {
	if (Mask & XUSBPS_IXR_UI_MASK) {
		if (isSetupComplete) {
			XUsbPs_dQHInvalidateCache(UsbInstance.HostConfig.QueueHead[1].pQH);

			XUsbPs_dTDInvalidateCache(qTDPollReceiver); // Invalidate before CPU read ??output buff??~~~~~~~~~~~~~~
			u8 *buffInput = (u8 *) XUsbPs_ReaddTD(qTDPollReceiver,
					XUSBPS_qTDANLP) - 1;

			bool isLMB = *buffInput & MOUSE_LMB_MASK;
			bool isRMB = (*buffInput & MOUSE_RMB_MASK) >> 1;

#if WILLIAMMOUSE == 1
			short *changeX = (short *) (buffInput + 1);
			short *changeY = (short *) (buffInput + 3);
#else
			short *changeX = (short *)(buffInput + 2);
			short *changeY = (short *)(buffInput + 4);
#endif

			UpdateMouse(isLMB, isRMB, *changeX, *changeY);
			//xil_printf("LMB = %d, RMB = %d, X = %4d, Y = %4d\r\n", isLMB, isRMB, *changeX, *changeY);

			qTDPollReceiver = USB_qTDActivateIn(
					&UsbInstance.HostConfig.QueueHead[1], false, 0);
			return;
		}

		isSetupComplete = USB_SetupDevice(&UsbInstance, status);

		if (isSetupComplete) {
			qTDPollReceiver = USB_SetupPolling(&UsbInstance);
		}

		if (status == 10) {
			status = 0;
			return;
		}

		status++;
		return;
	}

	if ((XUsbPs_ReadReg(UsbInstance.Config.BaseAddress, XUSBPS_PORTSCR1_OFFSET)
			& XUSBPS_PORTSCR_CCS_MASK) == 0) {
		xil_printf("Device Disconnected!\r\n\n");
		isSetupComplete = false;
		isPolling = false;
		status = 0;

		// Disable Periodic Schedule
		XUsbPs_ClrBits(&UsbInstance, XUSBPS_CMD_OFFSET, XUSBPS_CMD_PSE_MASK);

		// Disable Async
		XUsbPs_ClrBits(&UsbInstance, XUSBPS_CMD_OFFSET, XUSBPS_CMD_ASE_MASK);

		return;
	}
	xil_printf("Device Connected! ");

	if ((XUsbPs_ReadReg(UsbInstance.Config.BaseAddress, XUSBPS_PORTSCR1_OFFSET)
			& XUSBPS_PORTSCR_PE_MASK) == 0) {
		xil_printf("Resetting...\r\n\n");
		XUsbPs_SetBits(&UsbInstance, XUSBPS_PORTSCR1_OFFSET,
				XUSBPS_PORTSCR_PR_MASK);
		return;
	}

	u32 deviceSpeed =
			(XUsbPs_ReadReg(UsbInstance.Config.BaseAddress,
					XUSBPS_PORTSCR1_OFFSET)
					& XUSBPS_PORTSCR_PSPD_MASK) >> 26;

	if (deviceSpeed == 0) {
		xil_printf("Device operating at Full Speed.\r\n\n");
	} else if (deviceSpeed == 1) {
		xil_printf(
				"Device operating at Low Speed. Not supported - ignoring!\r\n\n");
		return;
	} else if (deviceSpeed == 2) {
		xil_printf(
				"Device operating at High Speed. Not supported - ignoring!\r\n\n");
		return;
	} else if (deviceSpeed == 3) {
		xil_printf("Device not connected.\r\n\n");
		return;
	}

//	USB_WriteSetupBuffer(buffSetup, 0, XUSBPS_REQ_SET_ADDRESS, 0xC, 0, 0); // Set Address 0x0C
//	USB_WriteSetupBuffer(buffSetup, 0x80, XUSBPS_REQ_GET_DESCRIPTOR, 0x0200, 0, 0x54); // Get Config

	isSetupComplete = USB_SetupDevice(&UsbInstance, status);

	// Enable Async
	XUsbPs_SetBits(&UsbInstance, XUSBPS_CMD_OFFSET, XUSBPS_CMD_ASE_MASK);
}

void VBlankIntrHandler(void *InstancePtr) {
	//xil_printf("60fps?");
}

//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main(void) {
	int status;
	//----------------------------------------------------
	// INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
	//----------------------------------------------------
	// Initialize Push Buttons
	status = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
	if (status != XST_SUCCESS)
		return XST_FAILURE;
	// Set all buttons direction to inputs
	XGpio_SetDataDirection(&BTNInst, 1, 0xFF);

	initTimer();

	// Initialize interrupt controller

	status = IntcInitFunction(INTC_DEVICE_ID, &BTNInst);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	XTtcPs_Start(&Timer);

	int usbStatus;

	// Run the USB setup
	usbStatus = USB_Setup(&INTCInst, &UsbInstance,
	XPAR_XUSBPS_0_DEVICE_ID, XPAR_XUSBPS_0_INTR, &UsbIntrHandler);

	// Set Output Buffer as Non-Cacheable
	for (int i = 0; i <= 896; i++) {
		Xil_SetTlbAttributes((INTPTR) (image_output_buffer + i * 2048),
				NORM_NONCACHE);
	}

	loadSprites();
	screen = SCREEN_MENU;
	DrawMenu();

	InitHdmi();

	while (1) {
		main_menu();
	}

	return 0;
}

//----------------------------------------------------
// INITIAL SETUP FUNCTIONS
//----------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr) {
	// Enable interrupt
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
	XGpio_InterruptGlobalEnable(&BTNInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			XScuGicInstancePtr);
	Xil_ExceptionEnable();

	return XST_SUCCESS;

}

int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr) {
	XScuGic_Config *IntcConfig;
	int status;

	// Interrupt controller initialisation
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig,
			IntcConfig->CpuBaseAddress);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	// Call to interrupt setup
	status = InterruptSystemSetup(&INTCInst);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	// Connect GPIO interrupt to handler
	status = XScuGic_Connect(&INTCInst,
	INTC_GPIO_INTERRUPT_ID, (Xil_ExceptionHandler) BTN_Intr_Handler,
			(void *) GpioInstancePtr);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	// Enable GPIO interrupts interrupt
	XGpio_InterruptEnable(GpioInstancePtr, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);

	//set up the timer interrupt
	XScuGic_Connect(&INTCInst, TIMER_IRPT_INTR,
			(Xil_ExceptionHandler) TimerIntrHandler, (void *) &Timer);

	//enable the interrupt for the Timer at GIC
	XScuGic_Enable(&INTCInst, TIMER_IRPT_INTR);

	//enable interrupt on the timer
	XTtcPs_EnableInterrupts(&Timer, XTTCPS_IXR_INTERVAL_MASK);

	XScuGic_SetPriorityTriggerType(&INTCInst, VBLANK_INTR, 0xA0, 0x3);

	//Connect the interrupt handler to the GIC.
	status = XScuGic_Connect(&INTCInst, VBLANK_INTR, (Xil_ExceptionHandler) VBlankIntrHandler, 0);
	if (status != XST_SUCCESS) {
		return status;
	}

	//Enable the interrupt for this specific device.
	XScuGic_Enable(&INTCInst, VBLANK_INTR);

	return XST_SUCCESS;
}
