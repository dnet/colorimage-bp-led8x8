#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define NUM_ROWS 8
#define ROW_PORT PORTD
#define ROW_DDR DDRD
#define ROW_OFFSET 2
#define COL_PORT PORTB

volatile uint8_t framebuf[NUM_ROWS];

#define TIMER1_PRESCALE_1 1
#define TIMER1_PRESCALE_8 2
#define TIMER1_PRESCALE_64 3
#define TIMER1_PRESCALE_256 4
#define TIMER1_PRESCALE_1024 5

uint8_t cur_row = 0;
uint8_t ROW_MASK = 0xFF;

ISR( TIMER1_COMPA_vect ) {
	if (cur_row++ >= NUM_ROWS) cur_row = 0;
	PORTD &= ROW_MASK;
	PORTB = framebuf[cur_row];
	PORTD |= cur_row << ROW_OFFSET;
}

#define  LEFT_SIDE 0x00
#define RIGHT_SIDE 0x10

ISR( USART_RX_vect  ) {
	uint8_t incoming, row, side, mask, pixels;
	
	incoming = UDR;
	row = incoming >> 5;
	side = incoming & 0x10;
	mask = (side == RIGHT_SIDE) ? 0xF0 : 0x0F;
	pixels = incoming & 0x0F;
	
	if (side == LEFT_SIDE) pixels <<= 4;
	
	framebuf[row] = (framebuf[row] & mask) | pixels;
}

int main(void) {

	DDRB = 0xFF;
	DDRD |= 4 + 8 + 16;

	for (uint8_t row = 0; row < NUM_ROWS; row++) {
		framebuf[row] = 0xFF;
		ROW_MASK &= ~(_BV(ROW_OFFSET) << row);
	}

	const uint16_t brr = F_CPU / 16 / 9600 - 1;
	UBRRL = brr & 0xFF;
	UBRRH = brr >> 8;
	UCSRB |= _BV(RXEN) | _BV(RXCIE);

	TCCR1B = (1 << WGM12) | TIMER1_PRESCALE_1;
	OCR1A = (uint16_t)5000;

	TIMSK |= 1 << OCIE1A;   // Output Compare Interrupt Enable (timer 1, OCR1A)
	sei();                 // Set Enable Interrupts

	while (1);
}
