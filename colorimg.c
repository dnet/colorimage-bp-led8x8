#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define NUM_ROWS 8
#define ROW_PORT PORTD
#define ROW_DDR DDRD
#define ROW_OFFSET 2
#define COL_PORT PORTB
#define COL_DDR DDRB
#define COL_DATA 1
#define COL_CLOCK 2
#define ROW_MASK ((NUM_ROWS - 1) << ROW_OFFSET)

volatile uint8_t framebuf[NUM_ROWS];

#define TIMER1_PRESCALE_1 1
#define TIMER1_PRESCALE_8 2
#define TIMER1_PRESCALE_64 3
#define TIMER1_PRESCALE_256 4
#define TIMER1_PRESCALE_1024 5

uint8_t cur_row = 0;

ISR( TIMER1_COMPA_vect ) {
	uint8_t bit, fbrow;
	if (++cur_row >= NUM_ROWS) cur_row = 0;
	fbrow = framebuf[cur_row];
	for (bit = 0x80; bit; bit >>= 1) {
		COL_PORT = (fbrow & bit) == bit ? COL_DATA : 0;
		COL_PORT |= COL_CLOCK;
		COL_PORT &= ~COL_CLOCK;
	}
	ROW_PORT = (ROW_PORT & ~ROW_MASK) | (cur_row << ROW_OFFSET);
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

	COL_DDR |= COL_DATA | COL_CLOCK;
	ROW_DDR |= ROW_MASK;

	for (uint8_t row = 0; row < NUM_ROWS; row++) {
		framebuf[row] = row % 2 ? 0x55 : 0xAA;
	}

	const uint16_t brr = F_CPU / 16 / 9600 - 1;
	UBRRL = brr & 0xFF;
	UBRRH = brr >> 8;
	UCSRB |= _BV(RXEN) | _BV(RXCIE);

	TCCR1B = (1 << WGM12) | TIMER1_PRESCALE_1;
	OCR1A = (uint16_t)20000;

	TIMSK |= 1 << OCIE1A;   // Output Compare Interrupt Enable (timer 1, OCR1A)
	sei();                 // Set Enable Interrupts

	while (1);
}
