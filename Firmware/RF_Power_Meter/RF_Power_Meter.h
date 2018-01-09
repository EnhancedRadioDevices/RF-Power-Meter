/* Enhanced Radio Devices */
/* 2018 by Nigel Vander Houwen <nigel@enhancedradio.com> */

#ifndef _RF_Power_Meter_H_
#define _RF_Power_Meter_H_

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Includes
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#include "Descriptors.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Macros
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Enable ANSI color codes to be sent. Uses a small bit of extra program space for 
// storage of color codes/modified strings.
#define ENABLECOLORS

#define SOFTWARE_STR "\r\nERD RF Power Meter"
#define HARDWARE_VERS "1.2"
#define SOFTWARE_VERS "1.0"

// Serial input
#define DATA_BUFF_LEN 32

// ADC
#define ADC_V_REF 1.2
#define ADC_AVG_POINTS 5

// Pins
#define RF_ANALOG PF0
#define LED PF6

// Schedule
#define READ_RF_DELAY 4 // Ticks. ~1s

// EEPROM Offsets


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Globals
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Timer
volatile unsigned long timer = 0;
// Schedule
volatile uint8_t schedule_read_rf = 0;

// Standard file stream for the CDC interface when set up, so that the
// virtual CDC COM port can be used like any regular character stream
// in the C APIs.
static FILE USBSerialStream;

// Help string
const char STR_Help_Info[] PROGMEM = "\r\nVisit https://github.com/EnhancedRadioDevices/RF-Power-Meter for full docs.";

// Reused strings
#ifdef ENABLECOLORS
	const char STR_Unrecognized[] PROGMEM = "\r\n\x1b[31mINVALID COMMAND\x1b[0m";
#else
	const char STR_Unrecognized[] PROGMEM = "\r\nINVALID COMMAND";
#endif	

const char STR_Backspace[] PROGMEM = "\x1b[D \x1b[D";

// Command strings
const char STR_Command_HELP[] PROGMEM = "HELP";
const char STR_Command_DEBUG[] PROGMEM = "DEBUG";

// State Variables
char * DATA_IN;
uint8_t DATA_IN_POS = 0;
uint8_t BOOT_RESET_VECTOR = 0;

/** LUFA CDC Class driver interface configuration and state information.
 * This structure is passed to all CDC Class driver functions, so that
 * multiple instances of the same class within a device can be
 * differentiated from one another.
 */ 
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
	.Config = {
		.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
		.DataINEndpoint           = {
			.Address          = CDC_TX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.DataOUTEndpoint = {
			.Address          = CDC_RX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.NotificationEndpoint = {
			.Address          = CDC_NOTIFICATION_EPADDR,
			.Size             = CDC_NOTIFICATION_EPSIZE,
			.Banks            = 1,
		},
	},
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Prototypes
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Set up a fake function that points to a program address where the bootloader should be
// based on the part type.
#ifdef __AVR_ATmega32U4__
	void (*bootloader)(void) = 0x3800;
#endif

// USB
static inline void run_lufa(void);

// EEPROM Read & Write
static inline void EEPROM_Reset(void);

// DEBUG
static inline void DEBUG_Dump(void);

// ADC
static inline int16_t ADC_Read_RF(void);

// LED
static inline void Set_LED(int8_t state);

// Output
static inline void printPGMStr(PGM_P s);
static inline void PRINT_Status(void);
static inline void PRINT_Help(void);

// Input
static inline void INPUT_Clear(void);
static inline void INPUT_Parse(void);

// Watchdog
static inline void Watchdog_Disable(void);
static inline void Watchdog_Enable(void);

#endif
