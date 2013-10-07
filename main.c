#include "main.h"
#include "uart.h"
#include "console.h"
#include "appdb.h"
#include "ds1302.h"
#include "efs.h"
#include "flash.h"
#include "lib.h"

#define RECVBUFLEN 64

unsigned char prompt[] PROGMEM = "\x0D\x0AM88>";
unsigned char recvbuf[RECVBUFLEN];
unsigned char token_count;
unsigned char* tokenptrs[MAXTOKENS];


static void noints(void) {
	EIMSK = 0;
	EIFR = 0x03;
	PCICR = 0;
	PCMSK2 = 0;
	PCMSK1 = 0;
	PCMSK0 = 0;
	sei();
	}
	
int main(void) {
	void(*func)(void);
	cli();
	noints();
	uart_init();
	sendstr_P(PSTR("UART INIT OK\r\nRTC INIT"));
	rtc_init();
	sendstr_P(PSTR(" OK\r\nFLASH INIT"));
	flash_init();
	sendstr_P(PSTR(" OK"));
	power_adc_disable();
	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();
	power_twi_disable();
	for (;;) {
		sendstr_P((PGM_P)prompt);
		getline(recvbuf,RECVBUFLEN);
		tokenize(recvbuf,tokenptrs, &token_count);
		if (token_count) {
			func = find_appdb(tokenptrs[0]);
			func();
		}
		}
	}


