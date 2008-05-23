/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

/*
	Keyboard demonstration application by Denver Gingerich.

	This example is based on the MyUSB Mouse demonstration application,
	written by Dean Camera.
*/

/*
	Keyboard demonstration application, using endpoint interrupts. This
	gives a simple reference application for implementing a USB Keyboard
	using the basic USB HID drivers in all modern OSes (i.e. no special
	drivers required). Control requests are also fully interrupt driven.
	
	On startup the system will automatically enumerate and function
	as a keyboard when the USB connection to a host is present. To use
	the keyboard example, manipulate the joystick to send the letters
	a, b, c, d and e. See the USB HID documentation for more information
	on sending keyboard event and keypresses.
*/

/*
	USB Mode:           Device
	USB Class:          Human Interface Device (HID)
	USB Subclass:       Keyboard
	Relevant Standards: USBIF HID Standard
	                    USBIF HID Usage Tables 
	Usable Speeds:      Low Speed Mode, Full Speed Mode
*/

#include "KeyboardFullInt.h"

/* Project Tags, for reading out using the ButtLoad project */
BUTTLOADTAG(ProjName,     "MyUSB KeyboardFI App");
BUTTLOADTAG(BuildTime,    __TIME__);
BUTTLOADTAG(BuildDate,    __DATE__);
BUTTLOADTAG(MyUSBVersion, "MyUSB V" MYUSB_VERSION_STRING);

int main(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable Clock Division */
	SetSystemClockPrescaler(0);

	/* Hardware Initialization */
	Joystick_Init();
	LEDs_Init();
	
	/* Indicate USB not ready */
	LEDs_SetAllLEDs(LEDS_LED1 | LEDS_LED3);

	/* Initialize USB Subsystem */
	USB_Init();
	
	/* Main program code loop */
	for (;;)
	{
		/* No main code -- all USB code is interrupt driven */
	}
}

EVENT_HANDLER(USB_Connect)
{
	/* Indicate USB enumerating */
	LEDs_SetAllLEDs(LEDS_LED1 | LEDS_LED4);
}

EVENT_HANDLER(USB_Reset)
{
	/* Select the control endpoint */
	Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

	/* Enable the endpoint SETUP interrupt ISR for the control endpoint */
	USB_INT_Enable(ENDPOINT_INT_SETUP);
}

EVENT_HANDLER(USB_Disconnect)
{
	/* Indicate USB not ready */
	LEDs_SetAllLEDs(LEDS_LED1 | LEDS_LED3);
}

EVENT_HANDLER(USB_ConfigurationChanged)
{
	/* Setup Keyboard Keycode Report Endpoint */
	Endpoint_ConfigureEndpoint(KEYBOARD_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_IN, KEYBOARD_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	/* Enable the endpoint IN interrupt ISR for the report endpoint */
	USB_INT_Enable(ENDPOINT_INT_IN);

	/* Setup Keyboard LED Report Endpoint */
	Endpoint_ConfigureEndpoint(KEYBOARD_LEDS_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_OUT, KEYBOARD_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	/* Enable the endpoint OUT interrupt ISR for the LED report endpoint */
	USB_INT_Enable(ENDPOINT_INT_OUT);

	/* Indicate USB connected and ready */
	LEDs_SetAllLEDs(LEDS_LED2 | LEDS_LED4);
}

ISR(ENDPOINT_PIPE_vect)
{
	/* Check if the control endpoint has recieved a request */
	if (Endpoint_HasEndpointInterrupted(ENDPOINT_CONTROLEP))
	{
		/* Clear the endpoint interrupt */
		Endpoint_ClearEndpointInterrupt(ENDPOINT_CONTROLEP);

		/* Process the control request */
		USB_USBTask();

		/* Handshake the endpoint setup interrupt - must be after the call to USB_USBTask() */
		USB_INT_Clear(ENDPOINT_INT_SETUP);
	}

	/* Check if keyboard endpoint has interrupted */
	if (Endpoint_HasEndpointInterrupted(KEYBOARD_EPNUM))
	{
		USB_KeyboardReport_Data_t KeyboardReportData = {Modifier: 0, KeyCode: 0};
		uint8_t                   JoyStatus_LCL      = Joystick_GetStatus();

		if (JoyStatus_LCL & JOY_UP)
		  KeyboardReportData.KeyCode = 0x04; // A
		else if (JoyStatus_LCL & JOY_DOWN)
		  KeyboardReportData.KeyCode = 0x05; // B

		if (JoyStatus_LCL & JOY_LEFT)
		  KeyboardReportData.KeyCode = 0x06; // C
		else if (JoyStatus_LCL & JOY_RIGHT)
		  KeyboardReportData.KeyCode = 0x07; // D

		if (JoyStatus_LCL & JOY_PRESS)
		  KeyboardReportData.KeyCode = 0x08; // E

		/* Clear the Keyboard Report endpoint interrupt and select the endpoint */
		Endpoint_ClearEndpointInterrupt(KEYBOARD_EPNUM);
		Endpoint_SelectEndpoint(KEYBOARD_EPNUM);

		/* Write Keyboard Report Data */
		Endpoint_Write_Byte(KeyboardReportData.Modifier);
		Endpoint_Write_Byte(0x00);
		Endpoint_Write_Byte(KeyboardReportData.KeyCode);			

		/* Handshake the IN Endpoint - send the data to the host */
		Endpoint_FIFOCON_Clear();
		
		/* Clear the endpoint IN interrupt flag */
		USB_INT_Clear(ENDPOINT_INT_IN);
	}

	/* Check if Keyboard LED status Endpoint has interrupted */
	if (Endpoint_HasEndpointInterrupted(KEYBOARD_LEDS_EPNUM))
	{
		/* Clear the endpoint OUT interrupt flag */
		USB_INT_Clear(ENDPOINT_INT_OUT);

		/* Clear the Keyboard LED Report endpoint interrupt and select the endpoint */
		Endpoint_ClearEndpointInterrupt(KEYBOARD_LEDS_EPNUM);
		Endpoint_SelectEndpoint(KEYBOARD_LEDS_EPNUM);

		/* Read in the LED report from the host */
		uint8_t LEDStatus = Endpoint_Read_Byte();
		uint8_t LEDMask   = LEDS_LED2;
		
		if (LEDStatus & 0x01) // NUM Lock
		  LEDMask |= LEDS_LED1;
		
		if (LEDStatus & 0x02) // CAPS Lock
		  LEDMask |= LEDS_LED3;

		if (LEDStatus & 0x04) // SCROLL Lock
		  LEDMask |= LEDS_LED4;

		/* Set the status LEDs to the current Keyboard LED status */
		LEDs_SetAllLEDs(LEDMask);

		/* Handshake the OUT Endpoint - clear endpoint and ready for next report */
		Endpoint_FIFOCON_Clear();
	}
}
