#include "main.h"
#include "flash.h"

unsigned char SectsIn256=1;



void flash_initialize(void) {
	unsigned int v;
	v = identify_flash();
	if (v == 0x45DA) { // The Winbond W29C020C
		SectsIn256 = 2;
	} else { // The AT29C020 - and try it with any other too :P
		SectsIn256 = 1;
	}
}


inline unsigned long int formaddr24(unsigned int sect, unsigned char offset) {
	unsigned long int rv;
	asm volatile(
	"mov %B0, %A1"	"\n\t"
	"mov %C0, %B1"	"\n\t"
	"mov %A0, %2"	"\n\t"
//	"clr %D0"	"\n\t"
	: "=&r" (rv)
	: "r" (sect), "r" (offset)
	);
	return rv;
}

unsigned char flash_databus_read(void) {
	unsigned char rv;
	rv = (PIND & 0xFC);
	rv |= (PINB & 0x03);
	return rv;
}

void flash_databus_tristate(void) {
	DDRB &= ~(_BV(0) | _BV(1));
	DDRD &= ~(_BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7));
	PORTB &= ~(_BV(0) | _BV(1));
	PORTD &= ~(_BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7));
}

void flash_databus_output(unsigned char data) {
	PORTB = ((PORTB & 0xFC) | (data & 0x03));
	PORTD = ((PORTD & 0x03) | (data & 0xFC));
	DDRB |= (_BV(0) | _BV(1));
	DDRD |= (_BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7));
}

void flash_init(void) {
	PORTC |= _BV(2);
	DDRC |= (_BV(2) | _BV(1) | _BV(0));
	PORTB &= ~_BV(2);
	DDRB |= (_BV(4) | _BV(2));
	// ADDR unit init done
	PORTC |= (_BV(3) | _BV(4) | _BV(5));
	DDRC |= (_BV(3) | _BV(4) | _BV(5));
	// control bus init done
	// hmm, i should probably tristate the data bus by default...
	flash_databus_tristate();
	// CE control is not absolutely necessary...
	flash_chip_enable();
}

// 'push' 3 bits of addr
void push_addr_bits(unsigned char bits) {
/*	if (bits&4) PORTB |= _BV(4);
	else PORTB &= ~(_BV(4));
	if (bits&1) PORTC |= _BV(1);
	else PORTC &= ~(_BV(1));
	if (bits&2) PORTC |= _BV(0);
	else PORTC &= ~(_BV(0));*/
	// SET THE BITS: (functionality near as-above)
	asm volatile(
	"sbi %0, 4\n\t"
	"sbi %1, 1\n\t"
	"sbi %1, 0\n\t"
	"sbrs %2, 2\n\t"
	"cbi %0, 4\n\t"
	"sbrs %2, 0\n\t"
	"cbi %1, 1\n\t"
	"sbrs %2, 1\n\t"
	"cbi %1, 0\n\t"
	::
	"I" (_SFR_IO_ADDR(PORTB)),
	"I" (_SFR_IO_ADDR(PORTC)),
	"r" (bits)
	);
	// double-toggle CP
	asm volatile(
	"sbi %0, 2\n\t"
	"sbi %0, 2\n\t"
	::
	"I" (_SFR_IO_ADDR(PINB))
	);
}

void flash_chip_enable(void) {
	PORTC &= ~_BV(3);
}

void flash_chip_disable(void) {
	PORTC |= _BV(3);
}

void flash_output_enable(void) {
	PORTC &= ~_BV(4);
}

void flash_output_disable(void) {
	PORTC |= _BV(4);
}

void flash_setaddr(unsigned long int addr) {
	unsigned char i,n,d;
	// Currently uses 18-bit addresses
	for (i=6;i>0;i--) { // as i isn't really used here, this way generates faster & smaller code
		asm volatile(
		"mov %0, %C1\n\t"
		"lsl %0 \n\t"
		"bst %B1, 7\n\t"
		"bld %0, 0\n\t"
		: "=r" (n)
		: "r" (addr)
		);
//		push_addr_bits((addr>>15)&0x07);
		push_addr_bits(n); // ABOVE REPLACED BY ASM
		d = 3;
		asm volatile(
		"lsl %A0\n\t"
		"rol %B0\n\t"
		"rol %C0\n\t"
		"dec %1\n\t"
		"brne .-10\n\t"
		: "+r" (addr), "+r" (d)
		:
		);
		// addr = (addr<<3); // Done as 24-bit op in above asm + d=3
	}
}
void flash_pulse_we(void) {
	asm volatile(
	"nop\n\t"
	"sbi %0, 5\n\t"
	"nop\n\t"
	"sbi %0, 5\n\t"
	::
	"I" (_SFR_IO_ADDR(PINC))
	);
}
	
	
void flash_wait_ready(void) {
	unsigned char a,b;
	a = flash_readcycle_single(0) & 0x40;
	
	while (1) {
		b = flash_readcycle_single(0) & 0x40;
		if (a == b) break;
		a = b;
	}
	
}
	
void flash_read_init(void) {
	flash_databus_tristate();
	flash_output_enable();
}


// assume chip enabled & output enabled & databus tristate
unsigned char flash_readcycle(unsigned long int addr) {
	flash_setaddr(addr);
	asm volatile(
	"nop\n\t"
	"nop\n\t"
	:: ); // 250 ns @ 8 mhz // assembler inspection shows that these shouldn't be necessary
	return flash_databus_read();
}

// assume only CE, and perform single cycle
unsigned char flash_readcycle_single(unsigned long int addr) {
	unsigned char data;
	flash_databus_tristate();
	flash_output_enable();
	flash_setaddr(addr);
	data = flash_databus_read();
	flash_output_disable();
	return data;
}

// assume only CE, perform single cycle
void flash_writecycle(unsigned long int addr, unsigned char data) {
	flash_output_disable();
	flash_databus_output(data);
	flash_setaddr(addr);
	flash_pulse_we();
}


void flash_chip_erase(void) {
	flash_writecycle(0x5555,0xAA);
	_delay_us(10);
	flash_writecycle(0x2AAA,0x55);
	_delay_us(10);
	flash_writecycle(0x5555,0x80);
	_delay_us(10);
	flash_writecycle(0x5555,0xAA);
	_delay_us(10);
	flash_writecycle(0x2AAA,0x55);
	_delay_us(10);
	flash_writecycle(0x5555,0x10);
	_delay_us(10);
	flash_wait_ready();
}


void flash_sector_erase(unsigned int sector) {
	unsigned char i,z=0;
	if (SectsIn256 == 2) z = 128;
	for(i=0;i<SectsIn256;i++) {
		flash_writecycle(0x5555,0xAA);
		_delay_us(10);
		flash_writecycle(0x2AAA,0x55);
		_delay_us(10);
		flash_writecycle(0x5555,0x80);
		_delay_us(10);
		flash_writecycle(0x5555,0xAA);
		_delay_us(10);
		flash_writecycle(0x2AAA,0x55);
		_delay_us(10);
		flash_writecycle((formaddr24(sector,i*z)),0x30);
		_delay_us(10);
		flash_wait_ready();
	}
}

void flash_db_dataset(unsigned char data) {
	PORTB = ((PORTB & 0xFC) | (data & 0x03));
	PORTD = ((PORTD & 0x03) | (data & 0xFC));
}

void flash_sector_write(unsigned int sector, unsigned char * buf) {
	unsigned char i;
	unsigned char n;
	unsigned char z=0;
	flash_output_disable();
	flash_databus_output(0);
	if (SectsIn256 == 2) z = 128;
	for(i=0;i<SectsIn256;i++) {
		/* Issue JEDEC Data Unprotect command */
		flash_writecycle(0x5555,0xAA);
		flash_writecycle(0x2AAA,0x55);
		flash_writecycle(0x5555,0xA0);
		n=0;
		while(1) {
			flash_setaddr(formaddr24(sector,i*z+n));
			flash_db_dataset(buf[i*z+n]);
			flash_pulse_we();
			n++;
			if (n == z) break;
		}
		flash_wait_ready();
	}
}
// Returns Vendor (LOW) and Device (High) ID
unsigned int identify_flash(void) {
	unsigned int rv;
	unsigned char device;
	unsigned char vendor;
	
	flash_writecycle(0x5555,0xAA);
	_delay_us(10);
	flash_writecycle(0x2AAA,0x55);
	_delay_us(10);
	flash_writecycle(0x5555,0x90);
	_delay_ms(10);
	device = flash_readcycle_single(1);
	vendor = flash_readcycle_single(0);
	asm volatile(
	"mov %A0, %2"		"\n\t"
	"mov %B0, %1"		"\n\t"
	: "=&r" (rv)
	: "r" (device), "r" (vendor)
	);
	//rv = ((device<<8)|(vendor)); // IN ABOVE ASM
	flash_writecycle(0x5555,0xAA);
	_delay_us(10);
	flash_writecycle(0x2AAA,0x55);
	_delay_us(10);
	flash_writecycle(0x5555,0xF0);
	_delay_ms(10);
	return rv;
}
