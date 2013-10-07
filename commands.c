#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "appdb.h"
#include "ds1302.h"
#include "efs.h"
#include "flash.h"

static void sendcrlf(void) {
	sendstr_P(PSTR("\r\n"));
}

void echo_cmd(void) {
	unsigned char i;
	for (i=1;i<token_count;i++) {
		sendstr(tokenptrs[i]);
		SEND(' ');
	}
}

static void uc2str2out_time(unsigned char val) {
	unsigned char buf[3];
	buf[0] = (val / 10) | 0x30;
	val = val % 10;
	buf[1] = (val | 0x30);
	buf[2] = 0;
	sendstr(buf);
}




static void tdcmd_impl(unsigned char s) {
	unsigned char a,b,c;
	if (token_count == 4) {
		/* SET TIME/DATE */
		a = str2uchar(tokenptrs[1]);
		b = str2uchar(tokenptrs[2]);
		c = str2uchar(tokenptrs[3]);
		if (s == ':') rtc_set_time(a,b,c);
		else rtc_set_date(a,b,c);
	}
	if (s == ':') {
		rtc_get_time(&a,&b,&c);
	} else {
		rtc_get_date(&a,&b,&c);
	}
	sendstr(tokenptrs[0]);
	sendstr_P(PSTR(": "));
	uc2str2out_time(a);
	SEND(s);
	uc2str2out_time(b);
	SEND(s);
	uc2str2out_time(c);
}

void time_cmd(void) {
	tdcmd_impl(':');
}

void date_cmd(void) {
	tdcmd_impl('.');
}

void rtcram_cmd(void) {
	unsigned char buf[4];
	unsigned char addr,val;
	if (token_count >= 2) { // READ OR VERIFY 
		addr = str2uchar(tokenptrs[1]);
		if (token_count == 3) {
			val = xstr2uchar(tokenptrs[2]);
			rtc_ram_write(addr,val);
			}
		sendstr_P(PSTR("RTC RAM ADDR "));
		uchar2str(buf,addr);
		sendstr(buf);
		sendstr_P(PSTR(" == "));
		val = rtc_ram_read(addr);
		uchar2xstr(buf,val);
		sendstr(buf);
		return;
	}
	// RTC RAM DUMP
	
	for (addr=0;addr<31;addr++) {
		val = rtc_ram_read(addr);
		uchar2xstr(buf,val);
		sendstr(buf);
		SEND(' ');
		if ((addr & 7) == 7) sendcrlf();
	}
	return;
}


unsigned long int calc_opdo(unsigned long int val1, unsigned long int val2, unsigned char *op) {
	switch (*op) {
		case '+':
			val1 += val2;
			break;
		case '-':
			val1 -= val2;
			break;
		case '*':
			val1 *= val2;
			break;
		case '/':
			val1 /= val2;
			break;
		case '%':
			val1 %= val2;
			break;
		case '&':
			val1 &= val2;
			break;
		case '|':
			val1 |= val2;
			break;
	}
	return val1;
}

void luint2outdual(unsigned long int val) {
	unsigned char buf[11];
	luint2str(buf,val);
	sendstr(buf);
	sendstr_P(PSTR(" ("));
	luint2xstr(buf,val);
	sendstr(buf);
	sendstr_P(PSTR("h) "));
}

unsigned long int closureparser(unsigned char firsttok, unsigned char*ptr) {
	unsigned char *op=NULL;
	unsigned char i,n;
	unsigned long int val1, val2;
	if (token_count <= firsttok) return 0;
	val1 = astr2luint(tokenptrs[firsttok]);
	sendstr_P(PSTR("{ "));
	luint2outdual(val1);
	n=0;
	for(i=firsttok+1;i<token_count;i++) {
		if (n&1) {
			sendstr(op);
			SEND(' ');
			if (*(tokenptrs[i]) == '(') {
				val2 = closureparser((i+1),&i);
			} else {
				val2 = astr2luint(tokenptrs[i]);
				luint2outdual(val2);
			}
			val1 = calc_opdo(val1,val2,op);
		} else {
			if (*(tokenptrs[i]) == ')') {
				sendstr_P(PSTR("} "));
				*ptr = i+1;
				return val1;
			}
			op = tokenptrs[i];
		}
		n++;
	}
	return val1;
}

void calc_cmd(void) {
	unsigned char *op=NULL;
	unsigned char i,n;
	unsigned long int val1;
	unsigned long int val2;
	if (token_count < 2) return;
	
	if (*(tokenptrs[1]) == '(') {
		val1 = closureparser(2,&i);
	} else {
		val1 = astr2luint(tokenptrs[1]);
		luint2outdual(val1);
		i=2;
	}
	n=0;
	for (;i<token_count;i++) {
		if (n&1) {
			sendstr(op);
			SEND(' ');
			if (*(tokenptrs[i]) == '(') {
				val2 = closureparser((i+1),&i);
			} else {
				val2 = astr2luint(tokenptrs[i]);
				luint2outdual(val2);
			}
			val1 = calc_opdo(val1,val2,op);
		} else {
			op = tokenptrs[i];
		}
		n++;
	}
	sendstr_P(PSTR("= "));
	luint2outdual(val1);
}

void help_cmd(void) {
	unsigned char i;
	struct command_t * ctptr;
	PGM_P name;
	for(i=0;;i++) {
		ctptr = &(appdb[i]);
		name = (PGM_P)pgm_read_word(&(ctptr->name));
		if (!name) break;
		sendstr_P(name);
		SEND(' ');
	}
}


void flash_readsect_cmd(void) {
	unsigned char i,d,z;
	unsigned char buf[3];
	unsigned char dbuf[16];
	unsigned long int addr;
	if (strlen((char*)tokenptrs[1]) != 4) return;
	addr = (((unsigned long int)xstr2uchar(tokenptrs[1]))<<16);
	addr |= (((unsigned long int)xstr2uchar((tokenptrs[1]+2)))<<8);
	flash_read_init();
	i=0;
	while(1) {
		d = flash_readcycle(addr|i);
		uchar2xstr(buf,d);
		sendstr(buf);
		SEND(' ');
		dbuf[i&0x0F] = d;
		if ((i & 0x0F) == 0x0F) {
			for(z=0;z<16;z++) {
				if (((dbuf[z]) < 32) || (dbuf[z] == 127) || (dbuf[z] > 127))
					SEND('.');
				else SEND(dbuf[z]);
			}
			sendcrlf();
		}
		i++;
		if (i == 0) break;
	}
	return;
}


void flash_readchip_cmd(void) {
	unsigned long int addr = 0;
	volatile unsigned char val;
	flash_read_init();
	for(addr=0;addr<0x40000;addr++) {
		val = flash_readcycle(addr);
	}
	sendstr_P(PSTR("Done"));
}


void flash_idchip_cmd(void) {
	unsigned char buf[5];
	unsigned int chipid;
	chipid = identify_flash();
	uint2xstr(buf,chipid);
	sendstr(buf);
	return;
}

static void flashi_packbits_decode(unsigned char*outbuf) {
	unsigned char outoffs;
	signed char countchar;
	unsigned char curchar;
	outoffs = 0;

	while(1) {
		countchar = (signed char)RECEIVE();
		
		if (countchar < 0) {
			countchar = (2) - countchar;
			curchar = RECEIVE();
			while (countchar) {
				outbuf[outoffs++] = curchar;
				countchar--;
			}
		} else {
			for (countchar++; countchar > 0; countchar--) {
				outbuf[outoffs++] = RECEIVE();
			}
		}
		if (outoffs == 0) break;
	}
	
}


static void flashi_interpret_read(unsigned char *fsectbuf, unsigned int sect) {
	unsigned char d=0;
	unsigned char n=0;
	unsigned char t=1;
	flash_read_init();
	while(1) {
		fsectbuf[n] = flash_readcycle(formaddr24(sect,n));
		if (n == 0) d = fsectbuf[n];
		else if (fsectbuf[n] != d) {
			t = 0;
			SEND(t);
			n++;
			while(1) {
				if ((n)&&(t==n)) {
					fsectbuf[n] = flash_readcycle(formaddr24(sect,n));
					n++;
				}
				SEND(fsectbuf[t]);
				t++;
				if (t == 0) return;
			}
		}
		n++;
		if (n == 0) break;
	}
	SEND(t);
	SEND(d);
	return;
}

static void flashi_interpret_write(unsigned char* fsectbuf, unsigned int sect) {
	unsigned char n,t;
	t = RECEIVE();
	switch (t) {
		case 1:
			n = RECEIVE();
			memset(fsectbuf,n,256);
			break;
		case 2:
			flashi_packbits_decode(fsectbuf);
			break;
		default:
			n=0;
			while(1) {
				fsectbuf[n] = RECEIVE();
				n++;
				if (n == 0) break;
				}
			break;
	}
	SEND(0x55);
	flash_sector_write(sect,fsectbuf);
}

void flasher_interface_cmd(void) {
	const unsigned char magic[4] = { 0xAA, 0x55, 0x00, 0xFF }; // MAGIC SEQUENCE AT INIT
	while (1) {
		if (RECEIVE() != magic[0]) continue;
		if (RECEIVE() != magic[1]) continue;
		if (RECEIVE() != magic[2]) continue;
		if (RECEIVE() != magic[3]) continue;
		break;
	}
	SEND(0xAA);
	flash_initialize();
	while (1) {
		unsigned char fsectbuf[256]; // FLASH SECTOR BUFFER
		unsigned char i;
		unsigned int sect;
		i = RECEIVE();
	
		switch(i) {
			case 0: // READ SECTOR
			case 1: // ERASE SECTOR
			case 2: // WRITE SECTOR
				sect = (((unsigned long int)RECEIVE()));
				sect |= (((unsigned long int)RECEIVE())<<8);
				switch(i) {
					case 0: //READ
						flashi_interpret_read(fsectbuf,sect);
						break;
					case 1: // ERASE
						flash_sector_erase(sect);
						SEND(0x55);
						break;
					case 2: // WRITE
						flashi_interpret_write(fsectbuf,sect);
						break;
				}
				break;

			case 3: // ERASE CHIP
				flash_chip_erase();
				SEND(0x55);
				break;
			case 0x7F: // QUIT FINT
				return;
			default:
				break;
		}
	}

}

#ifdef HAVE_GETKHZ
void getkhz_cmd(void) {
	unsigned int khz;
	unsigned char i;
	unsigned char buf[7];
	if (token_count != 2) return;
	i = str2uchar(tokenptrs[1]);
	khz = getkhz(i);
	uint2str(buf,khz);
	sendstr(buf);
}
#endif

#ifdef USE_EFS
static void efs_notfound(void) {
	sendstr_P(PSTR(": not found"));
	sendcrlf();
}


void efs_ls_cmd(void) {
	unsigned int size;
	unsigned int pointer=0xFFFE;
	unsigned char name[EFS_FNLEN+1];
	unsigned char i;
	if (token_count > 1) {
		for (i=1;i<token_count;i++) {
			pointer = efs_file_open(tokenptrs[i]);
			sendstr(tokenptrs[i]);
			if (pointer == 0xFFFF) { efs_notfound(); continue; };
			SEND(' ');
			size = efs_file_getsize(pointer);
			uint2str(name,size);
			sendstr(name);
			sendcrlf();
		}
		return;
	}
	while(1) {
		pointer = efs_file_getnextname(pointer,name);
		if (pointer == 0xFFFF) break;
		for (i=0;i<EFS_FNLEN;i++) if (name[i] == 0) name[i] = ' ';
		sendstr(name);
		SEND(' ');
		size = efs_file_getsize(pointer);
		uint2str(name,size);
		sendstr(name);
		sendcrlf();
	}
	sendstr_P(PSTR("--FREE SPACE: "));
	size = efs_get_freespace();
	uint2str(name,size);
	sendstr(name);
}

inline void efs_wr_outofspace(void) {
	 sendstr_P(PSTR("Out of space\r\n")); return; 
}

void efs_wr_cmd(void) {
	unsigned char linebuf[80];
	unsigned int handle;
	unsigned int len,i;
	if (token_count != 2) return;
	getline(linebuf,79);
	len = strlen((char*)linebuf)+1;
	linebuf[len] = 0;
	linebuf[len-1] = 0x0A;
	if (len == 1) { len = 0; }
	handle = efs_file_new(tokenptrs[1],len);
	if (len == 0) return;
	if (handle == 0xFFFF) { efs_wr_outofspace(); return; };
	efs_file_write(handle,linebuf,0,len);
	for(;;) {
		getline(linebuf,79);
		i = strlen((char*)linebuf)+1;
		linebuf[i] = 0;
		linebuf[i-1] = 0x0A;
		if (i == 1) return;
		handle = efs_file_chsize(handle,len+i);
		if (!(efs_file_write(handle,linebuf,len,i))) { efs_wr_outofspace(); return; };
		len += i;
	}
}

void efs_cat_cmd(void) {
	unsigned char i,d;
	unsigned int handle,size,n;
	for(i=1;i<token_count;i++) {
		handle = efs_file_open(tokenptrs[i]);
		if (handle == 0xFFFF) continue;
		size = efs_file_getsize(handle);
		for(n=0;n<size;n++) {
			efs_file_read(handle,&d,n,1);
			if (d == 0x0A) { sendcrlf(); continue; };
			SEND(d);
		}
	}
	
}

void efs_rm_cmd(void) {
	unsigned char i;
	unsigned int handle;
	for(i=1;i<token_count;i++) {
		handle = efs_file_open(tokenptrs[i]);
		if (handle == 0xFFFF) continue;
		efs_file_delete(handle);
	}
	
}



void efs_mv_cmd(void) {
	unsigned int handle,h2;
	if (token_count != 3) return;
	handle = efs_file_open(tokenptrs[2]);
	h2 = efs_file_open(tokenptrs[1]);
	if (h2 == 0xFFFF) { sendstr(tokenptrs[1]); efs_notfound(); return; }
	if (handle != 0xFFFF) efs_file_delete(handle);
	efs_file_chname(h2,tokenptrs[2]);
	return;
}

void efs_cp_cmd(void) {
	unsigned char *databuf;
	unsigned char i;
	unsigned int sizein, sizeout;
	unsigned int handlein, handleout;
	if (token_count < 3) return;
	sizein=0;
	sizeout=0;
	for(i=1;i<(token_count-1);i++) { 
		handlein = efs_file_open(tokenptrs[i]);
		if (handlein == 0xFFFF) {
			sendstr(tokenptrs[i]);
			efs_notfound();
			return;
		}
		handleout = efs_file_getsize(handlein);
		if (handleout > sizein) sizein = handleout;
		sizeout += handleout;
	}
	handleout = efs_file_new(tokenptrs[token_count-1],sizeout);
	if (handleout == 0xFFFF) { efs_wr_outofspace(); return; };
	databuf = alloca(sizein);
	if (!databuf) { sendstr_P(PSTR("OOME")); return; };
	sizeout = 0;
	for(i=1;i<(token_count-1);i++) {
		handlein = efs_file_open(tokenptrs[i]);
		sizein = efs_file_getsize(handlein);
		efs_file_read(handlein,databuf,0,sizein);
		efs_file_write(handleout,databuf,sizeout,sizein);
		sizeout += sizein;
	}
	return;
}
#endif
