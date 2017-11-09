/* (c) 2017 Enhanced Radio Devices */
//
// TODO
// ...

#include "RF_Power_Meter.h"

// Main Scheduling Interrupt
ISR(TIMER1_COMPA_vect){
	timer++;

	if ((timer) % READ_RF_DELAY == 0){ schedule_read_rf = 1; }

}

// Main program entry point.
int main(void) {
	// Store our reset vector for reference
	// See ATmega32u4 Datasheet p.59 for values
	BOOT_RESET_VECTOR = MCUSR;
	MCUSR = 0;

	// Initialize some variables
	int16_t BYTE_IN = -1;
	DATA_IN = malloc(DATA_BUFF_LEN);

	// Set up timer 1 for 0.25s interrupts
	TCCR1A = 0b00000000; // No pin changes on compare match
	TCCR1B = 0b00001010; // Clear timer on compare match, clock /8
	TCCR1C = 0b00000000; // No forced output compare
	OCR1A = 31250; // Set timer clear at this count value
	TCNT1 = 0;
	TIMSK1 = 0b00000010; // Enable interrupts on the A compare match

	Watchdog_Disable();
	wdt_reset();
	Watchdog_Enable();

	// Divide 16MHz crystal down to 1MHz for CPU clock.
	clock_prescale_set(clock_div_16);

	// Init USB hardware and create a regular character stream for the
	// USB interface so that it can be used with the stdio.h functions
	USB_Init();
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	// Enable interrupts
	GlobalInterruptEnable();

	// Print startup message
	printPGMStr(PSTR(SOFTWARE_STR));
	fprintf(&USBSerialStream, " V%s,%s", HARDWARE_VERS, SOFTWARE_VERS);
	run_lufa();

	// Configure LED Pin
	DDRF |= (1 << LED);
	Set_LED(0);

	// Enable the ADC
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // Enable ADC, clocked by /128 divider
	
	run_lufa();

	INPUT_Clear();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Main system loop
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	for (;;) {
		// Read a byte from the USB serial stream
		BYTE_IN = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

		// USB Serial stream will return <0 if no bytes are available.
		if (BYTE_IN >= 0) {
			// Echo the char we just received back out the serial stream so the user's 
			// console will display it.
			fputc(BYTE_IN, &USBSerialStream);

			Set_LED(1);

			// Switch on the input byte to determine what is is and what to do.
			switch (BYTE_IN) {
				case 8:
				case 127:
					// Handle Backspace chars.
					if (DATA_IN_POS > 0){
						DATA_IN_POS--;
						DATA_IN[DATA_IN_POS] = 0;
						printPGMStr(STR_Backspace);
					}
					break;

				case '\n':
				case '\r':
					// Newline, Parse our command
					INPUT_Parse();
					INPUT_Clear();
					break;

				case 3:
					// Ctrl-c bail out on partial command
					INPUT_Clear();
					break;
				
				case 29:
					// Ctrl-] reset all eeprom values
					EEPROM_Reset();
					INPUT_Clear();
					break;
					
				case 30:
					// Ctrl-^ jump into the bootloader
					// Disable the watchdog timer
					Watchdog_Disable();
					
					bootloader();
					break; // We should never get here...

				default:
					// Normal char buffering
					if (DATA_IN_POS < (DATA_BUFF_LEN - 1)) {
						DATA_IN[DATA_IN_POS] = BYTE_IN;
						DATA_IN_POS++;
						DATA_IN[DATA_IN_POS] = 0;
					} else {
						// Input is too long.
						printPGMStr(STR_Unrecognized);
						INPUT_Clear();
					}
					break;
			}
			
			Set_LED(0);
		}
		
		// Check for above threshold current usage
		if (schedule_read_rf) {
			// Collect ADC_AVG_POINTS samples and average them
			uint16_t average = 0;
			for (uint8_t i = 0; i < ADC_AVG_POINTS; i++) {
				average += ADC_Read_RF();
			}
			average = average / ADC_AVG_POINTS;
			
			// Convert the average reading into a voltage value referenced on ADC_V_REF
			float temp = (average * (ADC_V_REF / 1024.0));
			
			// Convert the voltage value into a dBm reading
			temp = (temp / 0.0173) - 68 + 19.95;
			
			fprintf(&USBSerialStream, "\r\n%.2f", temp);
			schedule_read_rf = 0;
		}
		
		// Keep the LUFA USB stuff fed regularly.
		run_lufa();
		
		// Reset the watchdog
		wdt_reset();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Command Parsing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Flush out our data input buffer, reset our position variable, and print a new prompt.
static inline void INPUT_Clear(void) {
	memset(&DATA_IN[0], 0, sizeof(DATA_IN));
	DATA_IN_POS = 0;
	
#ifdef ENABLECOLORS
	fprintf(&USBSerialStream, "\r\n\r\n# \x1b[36m>\x1b[0m ");
#else
	fprintf(&USBSerialStream, "\r\n\r\n# > ");
#endif
}

// We've gotten a new command, parse out what they want.
static inline void INPUT_Parse(void) {
	// HELP - Print a basic help menu
	if (strncasecmp_P(DATA_IN, STR_Command_HELP, 4) == 0) {
		PRINT_Help();
		return;
	}
	// DEBUG - Print a report of debugging information, including EEPROM variables
	if (strncasecmp_P(DATA_IN, STR_Command_DEBUG, 10) == 0) {
		DEBUG_Dump();
		return;
	}
	
	// If none of the above commands were recognized, print a generic error.
	printPGMStr(STR_Unrecognized);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Printing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Print a summary of all ports status'
static inline void PRINT_Status(void) {

}

// Print a quick help command
static inline void PRINT_Help(void) {
	printPGMStr(STR_Help_Info);
}

// Print a PGM stored string
static inline void printPGMStr(PGM_P s) {
	char c;
	while((c = pgm_read_byte(s++)) != 0) fputc(c, &USBSerialStream);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ EEPROM Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 

// Reset all EEPROM values to 255
static inline void EEPROM_Reset(void) {
	for (uint16_t i = 0; i < 512; i++) {
		eeprom_update_byte((uint8_t*)(i), 255);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Debugging Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Dump debugging data
static inline void DEBUG_Dump(void) {
	// Print hardware and software versions
	fprintf(&USBSerialStream, "\r\nV%s,%s", HARDWARE_VERS, SOFTWARE_VERS);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ LED Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Set the output LED state
static inline void Set_LED(int8_t state) {
	if (state == 1) {
		PORTF |= (1 << LED);
	} else {
		PORTF &= ~(1 << LED);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ ADC Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Read RF Power Value
static inline int16_t ADC_Read_RF(void) {
	ADMUX = 0b00000000;
	ADCSRB = 0b00000000;
	ADCSRA |= (1<<ADSC); // Start first conversion (throw away)
	while (ADCSRA & (1<<ADSC)); // Wait for conversion to complete
	ADCSRA |= (1<<ADSC); // Start second conversion (valid)
	while (ADCSRA & (1<<ADSC)); // Wait for conversion to complete
	return ADCW;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ USB Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Run the LUFA USB tasks (except reading)
static inline void run_lufa(void) {
	//CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

// Event handler for the library USB Connection event.
void EVENT_USB_Device_Connect(void) {
	// We're enumerated. Act on that as desired.
}

// Event handler for the library USB Disconnection event.
void EVENT_USB_Device_Disconnect(void) {
	// We're no longer enumerated. Act on that as desired.
}

// Event handler for the library USB Configuration Changed event.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	// USB is ready. Act on that as desired.
}

// Event handler for the library USB Control Request reception event.
void EVENT_USB_Device_ControlRequest(void) {
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Watchdog Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Disables the watchdog timer
static inline void Watchdog_Disable(void) {
	cli();
	wdt_reset();
	MCUSR &= ~(1 << WDRF);
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = 0x00;
	sei();
}

// Enables the watchdog timer
static inline void Watchdog_Enable(void) {
	cli();
	wdt_reset();
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = (1 << WDE) | (1 << WDP3) | (1 << WDP0);
	sei();
}