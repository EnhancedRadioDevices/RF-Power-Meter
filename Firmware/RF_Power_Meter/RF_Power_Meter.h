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
#define SOFTWARE_VERS "1.1"
#define EEPROM_VERS 1

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
// Calibration values
#define EEPROM_OFFSET_RF_CAL_SLOPE 0 // 1 byte * 27 Values (100MHz blocks) - Calibrate the frequency response slope
#define EEPROM_OFFSET_RF_CAL_INTERCEPT 27 // 1 byte * 27 Values (100MHz Blocks) - Calibrate the frequency response intercept
//#define EEPROM_OFFSET_NEXT 54
#define EEPROM_OFFSET_EEPROM_INIT 128

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
const char STR_Help_Info[] PROGMEM = "\r\n\"F<Frequency in MHz>\" to load the appropriate calibration values.\r\n\"R<1-10>\" to set the interval between readings (in seconds).\r\n\r\nVisit https://github.com/EnhancedRadioDevices/RF-Power-Meter for full docs.";

// Reused strings
#ifdef ENABLECOLORS
	const char STR_Unrecognized[] PROGMEM = "\r\n\x1b[31mINVALID COMMAND\x1b[0m";
	const char STR_Freq_Range[] PROGMEM = "\r\n\x1b[31mFrequency out of range. Using defaults.\x1b[0m";
#else
	const char STR_Unrecognized[] PROGMEM = "\r\nINVALID COMMAND";
	const char STR_Freq_Range[] PROGMEM = "\r\nFrequency out of range. Using defaults.";
#endif	

const char STR_Backspace[] PROGMEM = "\x1b[D \x1b[D";
const char STR_Load_Cal[] PROGMEM = "\r\nLoading calibration values for frequency: ";
const char STR_Rate_Set[] PROGMEM = "\r\nPrinting rate set to ";
const char STR_Slope_Set[] PROGMEM = "\r\nSlope set.";
const char STR_Intercept_Set[] PROGMEM = "\r\nIntercept set.";

// Command strings
const char STR_Command_HELP[] PROGMEM = "HELP";
const char STR_Command_DEBUG[] PROGMEM = "DEBUG";
const char STR_Command_SETSLOPE[] PROGMEM = "SETSLOPE";
const char STR_Command_SETINTERCEPT[] PROGMEM = "SETINTERCEPT";
const char STR_Command_OUTPUTRAW[] PROGMEM = "OUTPUTRAW";

// State Variables
char * DATA_IN;
uint8_t DATA_IN_POS = 0;
uint8_t BOOT_RESET_VECTOR = 0;
float RF_FREQ_SLOPE = 0.0;
uint8_t RF_FREQ_INTERCEPT = 0;
uint8_t PRINTING_RATE = 1;
uint8_t OUTPUTRAW = 0;

// Default calibration tables
const uint8_t RF_CAL_DEFAULTS_SLOPE[27] = \
		{173, 173, 173, 173, 173, 173, 173, 172, 172, 172, \
		 172, 172, 172, 172, 172, 171, 171, 171, 171, 171, \
		 171, 171, 171, 171, 172, 172, 172};
const uint8_t RF_CAL_DEFAULTS_INTERCEPT[27] = \
		{68, 68, 68, 68, 68, 69, 69, 69, 69, 70, \
		 70, 70, 70, 70, 70, 71, 70, 71, 73, 72, \
		 71, 71, 71, 71, 71, 72, 73};
		 
		 
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
static inline void EEPROM_Init(void);
static inline void EEPROM_Write_RF_Cal_Slope(uint8_t span, float value);
static inline float EEPROM_Read_RF_Cal_Slope(uint8_t span);
static inline void EEPROM_Write_RF_Cal_Intercept(uint8_t span, uint8_t value);
static inline uint8_t EEPROM_Read_RF_Cal_Intercept(uint8_t span);

// DEBUG
static inline void DEBUG_Dump(void);

// ADC
static inline int16_t ADC_Read_RF(void);
static inline void Load_RF_Calibration(uint16_t freq);

// LED
static inline void Set_LED(int8_t state);

// Output
static inline void printPGMStr(PGM_P s);
static inline void PRINT_Status(void);
static inline void PRINT_Help(void);

// Input
static inline long INPUT_Parse_num(void);
static inline void INPUT_Clear(void);
static inline void INPUT_Parse(void);

// Watchdog
static inline void Watchdog_Disable(void);
static inline void Watchdog_Enable(void);

#endif
