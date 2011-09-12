#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define PEEL_BAUD 2000000
#define PEEL_U2X
#define STATUS_LED (_BV(5)) // arduino 13
// Enable RX
#define enable_serial() (UCSR0B |= _BV(RXEN0))

#define id (SLAVE_ID << 4)

#if !defined(PEEL_BAUD)
#error Define PEEL_BAUD first
#endif

#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

#define XLAT_PORT PORTB
#define XLAT_DDR  DDRB
#define XLAT_PIN  PB1

// Events
// Keep the events in a low register, for fast access
// I used to use GPIOR0, but now I use that for the data.  Port C will do,
// but remember that bit 7 is unavailable, and bits 0 and 3 are used for the
// shift register.
#define events PORTC
namespace Event {
	static const uint8_t receiving_data = _BV(1);
};
// Received data will go here for processing by the main loop.
// (nothing seems to use r5...)
volatile register uint8_t rx_data asm("r5");

// How many bytes of data we've received for this row
#define rx_count GPIOR0

void die(uint8_t) __attribute__ ((noreturn));
void die(uint8_t status) {
	cli();
	// Turn off SPI, so we can control pin 13
	SPCR &= ~_BV(SPE);
	DDRB |= STATUS_LED;
//	printf("Error:%02x\n", status);
	for (uint8_t j = 0; j < 10; j++) {
		for (uint8_t i = 0; i < status; i++) {
			PORTB |= STATUS_LED;
			_delay_ms(20);
			PORTB &= ~STATUS_LED;
			_delay_ms(200);
		}
		_delay_ms(600);
	}
	// Try a reset... will this work?
	__asm__("jmp 0");
	// stop the compiler complaining that this function returns
	abort();
}

static uint8_t read_data() {
	// Check for hardware buffer overflow
	if (UCSR0A & _BV(DOR0))
		// This function never returns, so we don't care which registers
		// it clobbers
		die(2);

	return UDR0;
}

// the "static" seems to inline this function
static void handle_data(uint8_t data) {
	// Address byte
	if (data &= 1) {
		if ((data ^ id) == 1) {
			rx_count = 0;
			events |= Event::receiving_data;
		} else
			events &= ~Event::receiving_data;
	} else if (events & Event::receiving_data) {
		// Check that the previous byte was sent
		if (!(SPSR & _BV(SPIF)))
			die(4);
		SPDR = data;
		++rx_count;
		// is that it?
		if (rx_count >= 96) {
			XLAT_PORT |= _BV(XLAT_PIN);
			events &= Event::receiving_data;
			XLAT_PORT &= ~_BV(XLAT_PIN);
		}
	}
}

void init_blank_timer() {
	// Blank: activate every 4096 cycles
	// Set OC1B at bottom, clear at compare match
	TCCR1A = _BV(COM1B1);
	// CTC mode, TOP=ICR1
	TCCR1B = _BV(WGM13) | _BV(WGM12);
	// Blank every 4096 ticks
	ICR1 = 4096;
	// Clear BLANK when timer=1
	OCR1B = 1;
	// Timer on
	// No prescaling (timer runs at clock frequency), keeps the BLANK
	// signal short
	TCCR1B |= _BV(CS10);
}

void init_tlc_data() {
	// Enable SPI, Master, set clock rate fck/2
	SPCR = _BV(SPE) | _BV(MSTR);
	SPSR = _BV(SPI2X);
}

void init_xlat() {
	XLAT_DDR |= _BV(XLAT_PIN);
	XLAT_PORT &= ~_BV(XLAT_PIN);
}

void init_serial() {
  // Init serial
  UBRR0H = (uint8_t) (PEEL_UBRR_VAL >> 8);
  UBRR0L = (uint8_t) (PEEL_UBRR_VAL);
  // Asynchronous, no parity, 1 stop bit, 8 data bits
  UCSR0C = _BV(UPM01) | _BV(UCSZ01) | _BV(UCSZ00);
  // TX enable. Don't turn on RX yet - wait until we're ready to receive
  UCSR0B = _BV(TXEN0);
  UCSR0A = 0
#if defined(SERIAL_U2X)
	| _BV(U2X0)
#endif
  ;
}

int main(void) {
	init_tlc_data();
	init_serial();
	init_blank_timer();
	init_xlat();

	// Send a test message
	UDR0 = 'X';

	// Wait until here to enable the serial port, so data already on the
	// line doesn't overflow the rx buffer
	enable_serial();

	while(true) {
		// is there serial data?
		if (UCSR0A & _BV(RXC0))
			handle_data(read_data());
	}

	return 0;
}