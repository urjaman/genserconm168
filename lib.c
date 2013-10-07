#include "main.h"
#include "lib.h"
#include "ds1302.h"

/*void luint2str(unsigned char *buf, unsigned long int val) {
	unsigned long int divisor;
	unsigned char flag;
	unsigned char mark;
	flag=0;
	
	for(divisor=1000000000;divisor>1;divisor/=10) {
		mark = ((val / divisor) | 0x30);
		val = (val % divisor);
		if ((mark != 0x30)||(flag)) {*buf = mark; flag=1; buf++; };
	}
	mark = (val | 0x30);
	*buf = mark;
	buf++;
	*buf = 0;
}*/

void luint2str(unsigned char *buf, unsigned long int val) {
	ultoa(val,(char*)buf,10);
}


void uint2str(unsigned char *buf, unsigned int val) {
	luint2str(buf,(unsigned long int)val);
	}


void uchar2str(unsigned char *buf, unsigned char val) {
	uint2str(buf,(unsigned int)val);
	}
	
static unsigned char hextab_func(unsigned char offset) {
	offset |= 0x30;
	if (offset > 0x39) offset += 7;
	return offset;
}

void uchar2xstr(unsigned char *buf,unsigned char val) {
	unsigned char offset;
	offset = ((val>>4)&0x0F);
	buf[0] = hextab_func(offset);
	offset = (val&0x0F);
	buf[1] = hextab_func(offset);
	buf[2] = 0;
}
	
unsigned char str2uchar(unsigned char *buf) {
	unsigned char rv;
	for (rv=0;*buf;buf++) {
		if ((*buf >= '0')||(*buf <= '9')) {
			rv *= 10;
			rv = rv + (*buf &0x0F);
			
		}
		}
	return rv;
	}

static unsigned char reverse_hextab(unsigned char hexchar) {
	if (hexchar > 0x39) hexchar = hexchar - 7;
	hexchar &= 0x0F;
	return hexchar;
}

static unsigned char isvalid(unsigned char c, unsigned char base) {
	if (base == 16) {
		if ((c > 'F') || (c < '0')) return 0;
		if ((c > '9') && (c < 'A')) return 0;
	} else {
		if ((c > '9') || (c < '0')) return 0;
	}
	return 1;
}

unsigned long int astr2luint(unsigned char *buf) {
	unsigned char i;
	unsigned char len;
	unsigned char base=10;
	unsigned long int rv;
	strupr((char*)buf);
	len = strlen((char*)buf);
	if (buf[len-1] == 'H') base=16;

	rv = 0;
	for(i=0;i<len;i++) {
		if (!(isvalid(buf[i],base))) continue;
		rv = rv * base;
		rv += reverse_hextab(buf[i]); // RV HEXTAB works also for base 10
	}
	if (buf[0] == '~') rv = ~rv;
	return rv;
}




unsigned char xstr2uchar(unsigned char *buf) {
	unsigned char rv;
	rv = (reverse_hextab(*buf)<<4);
	buf++;
	rv |= reverse_hextab(*buf);
	return rv;
	}




/*void luint2xstr(unsigned char*buf, unsigned long int val) {
	unsigned char flag=0;
	unsigned char mark;
	unsigned char shift;
	
	for(shift=28;shift>0;shift -= 4) {
		mark = hextab_func(((val>>shift)&0x0F));
		if ((mark != 0x30)||(flag)) {
			*buf = mark; flag=1; buf++;
		}
	}
	*buf = hextab_func((val&0x0F));
	buf++;
	*buf = 0;
}*/

void luint2xstr(unsigned char *buf, unsigned long int val) {
	ultoa(val,(char*)buf,16);
	strupr((char*)buf); // i dont like "aaaah"...
}

void uint2xstr(unsigned char *buf,unsigned int val) {
	luint2xstr(buf,(unsigned long int)val);
}


unsigned char bcd2bin(unsigned char bcd) {
        return ((bcd>>4)*10)+(bcd&0x0F);
        }
unsigned char bin2bcd(unsigned char bin) {
        return (((bin/10)<<4)|(bin%10));
        }

static unsigned char timediff(unsigned char h1, unsigned char m1, unsigned char s1, unsigned char h2, unsigned char m2, unsigned char s2) {
	unsigned long int time1, time2;
	time1 = (((unsigned long int)h1)*60*60)+ (((unsigned long int)m1)*60) + s1;
	time2 = (((unsigned long int)h2)*60*60)+ (((unsigned long int)m2)*60) + s2;
	if (time1 > time2) time2 += 24L*60L*60L;
	return ((unsigned char)(time2 - time1));
}

unsigned int getkhz(unsigned char i) {
	unsigned char timetaken;
	unsigned char h1,m1,s1,h2,m2,s2;
	unsigned int myrealkhz;
	const unsigned int mykhz = (F_CPU/1000);
	unsigned char realtime; // s
	unsigned int ratio;
	realtime = 5*i;
	cli();
	rtc_get_time(&h1,&m1,&s1);
	for (;i>0;i--) _delay_ms(5000);
	rtc_get_time(&h2,&m2,&s2);
	sei();
	timetaken = timediff(h1,m1,s1,h2,m2,s2);
	ratio = (((unsigned long int)realtime)*1000L) / timetaken;
	myrealkhz = (((unsigned long int)mykhz) * ratio) / 1000L;
	return myrealkhz;
}

void autocalibrate(void) {
	unsigned char i,n;
	const unsigned int trykhz = (F_CPU/1000);
	unsigned int realkhz;
	unsigned int khzoffs;
	const unsigned char maxtrys = 10;
	const unsigned char speed = 30;
	for(n=0;n<maxtrys;n++) {
		realkhz = getkhz(speed);
		i = OSCCAL;
		if ((realkhz < (trykhz+(trykhz/100)))&&(realkhz > (trykhz-(trykhz/100)))) { // 1% accuracy reached
			rtc_ram_write(28,(trykhz&0xFF));
			rtc_ram_write(29,((trykhz>>8)&0xFF));
			rtc_ram_write(30,i);
			return;
		}
		if (realkhz > trykhz) { 
			khzoffs = realkhz - trykhz;
			i -= (1+(khzoffs/128));
			if ((i < 128)&&(i>96)) i -= 32;
		} else {
			khzoffs = trykhz - realkhz;
			i += (1+(khzoffs/128));
			if ((i > 96)&&(i<128)) i += 32;
		}
		OSCCAL = i;
	}
	return;	
}
