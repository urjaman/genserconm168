#include "main.h"
#include "efs.h"

/*
#include "uart.h"
#include "console.h"
#include "lib.h"

unsigned char erb(unsigned char* ptr) {
	unsigned char buf[5];
	unsigned char d;
	d = eeprom_read_byte(ptr);
	sendstr_P(PSTR(" ERB 0x"));
	uint2xstr(buf,(unsigned int)ptr);
	sendstr(buf);
	sendstr_P(PSTR(" READ 0x"));
	uchar2xstr(buf,d);
	sendstr(buf);
	return d;
}

unsigned int erw(unsigned int* ptr) {
	unsigned char buf[5];
	unsigned int d;
	d = eeprom_read_word(ptr);
	sendstr_P(PSTR(" ERW 0x"));
	uint2xstr(buf,(unsigned int)ptr);
	sendstr(buf);
	sendstr_P(PSTR(" READ 0x"));
	uint2xstr(buf,d);
	sendstr(buf);
	return d;
}

void erx(unsigned char *rp, unsigned char*ep, unsigned int n) {
	unsigned char buf[6];
	unsigned int i;
	unsigned char val;
	eeprom_read_block(rp,ep,n);
	sendstr_P(PSTR(" ERX 0x"));
	uint2xstr(buf,(unsigned int)ep);
	sendstr(buf);
	sendstr_P(PSTR(" LEN "));
	uint2str(buf,n);
	sendstr(buf);
	for(i=0;i<n;i++) {
		SEND(' ');
		val = rp[i];
		uchar2xstr(buf,val);
		sendstr(buf);
	}
	SEND(' ');
}


//#define EFS_GETBYTE(addr) erb((unsigned char*)(EFSOFFSET+(addr)))
//#define EFS_GETWORD(addr) erw((unsigned int*)(EFSOFFSET+(addr)))
//#define EFS_GETBLOCK(rp,ep,n) erx(rp,(void*)(EFSOFFSET+(ep)),n)
*/

#define EFS_GETBYTE(addr) eeprom_read_byte((unsigned char*)(EFSOFFSET+(addr)))
#define EFS_GETWORD(addr) eeprom_read_word((unsigned int*)(EFSOFFSET+(addr)))
#define EFS_GETBLOCK(rp,ep,n) eeprom_read_block(rp,(void*)(EFSOFFSET+(ep)),n)


#define EFS_PUTBYTE(addr,val) eeprom_write_byte((unsigned char*)(EFSOFFSET+(addr)),val)
#define EFS_PUTWORD(addr,val) eeprom_write_word((unsigned int*)(EFSOFFSET+(addr)),val)
#define EFS_PUTBLOCK(rp,ep,n) eeprom_write_block(rp,(void*)(EFSOFFSET+(ep)),n)
#define EFS_FILEAREA 0
#define EFS_HEADERSZ (2+EFS_FNLEN)
#define SAFETY 0
// SET SAFETY 0 FOR RLY SMALL CODE, 1 FOR NORMAL CHECKS, 2 FOR PARANOIA


#ifdef USE_EFS

void efs_format(void) {
	EFS_PUTWORD(EFS_FILEAREA,0xFFFF);
}

static unsigned int efs_find_endpointer(void) {
	unsigned int pointer = EFS_FILEAREA;
	unsigned int data;
	for (;;) {
		data = EFS_GETWORD(pointer);
		if (data == 0xFFFF) return pointer;
		pointer += data+EFS_HEADERSZ;
		if (SAFETY>1) if (pointer >= NEWEFS_SZ) { efs_format(); return EFS_FILEAREA; };
	}
}

static unsigned char efs_validate_handle(unsigned int handle) {
	if (SAFETY) if (handle == 0xFFFF) return 1;
	if (SAFETY>1) if (handle+EFS_HEADERSZ >= NEWEFS_SZ) return 1;
	if (SAFETY>1) if (EFS_GETWORD(handle) == 0xFFFF) return 1;
	return 0;
}

unsigned int efs_file_open(unsigned char *fn) {
	unsigned int pointer = EFS_FILEAREA;
	unsigned int data;
	unsigned char name[EFS_FNLEN+1];
	name[EFS_FNLEN] = 0;
	for (;;) {
		data = EFS_GETWORD(pointer);
		if (data == 0xFFFF) return data;
		EFS_GETBLOCK(name,(pointer+2),EFS_FNLEN);
		if (strncmp((char*)fn,(char*)name,EFS_FNLEN) == 0) {
			return pointer;
		}
		pointer += data+EFS_HEADERSZ;
		if (SAFETY>1) if (pointer >= NEWEFS_SZ) { efs_format(); return 0xFFFF; };
	}
}

unsigned int efs_file_new(unsigned char *fn, unsigned int size) {
	unsigned int pointer;
	unsigned int data;
	unsigned char name[EFS_FNLEN+1];
	pointer = efs_file_open(fn);
	if (pointer != 0xFFFF) {
		efs_file_delete(pointer);
	}
	pointer = efs_find_endpointer();
	if (pointer+size+EFS_HEADERSZ+2 >= NEWEFS_SZ) return 0xFFFF;
	EFS_PUTWORD(pointer,size);
	memset(name,0,EFS_FNLEN+1);
	strncpy((char*)name,(char*)fn,EFS_FNLEN);
	EFS_PUTBLOCK(name,(pointer+2),EFS_FNLEN);
	data = pointer+EFS_HEADERSZ+size;
	EFS_PUTWORD(data,0xFFFF);
	return pointer;
}

void efs_file_chname(unsigned int handle, unsigned char *fn) {
	unsigned char name[EFS_FNLEN+1];
	if (efs_validate_handle(handle)) return;
	memset(name,0,EFS_FNLEN+1);
	strncpy((char*)name,(char*)fn,EFS_FNLEN);
	EFS_PUTBLOCK(name,(handle+2),EFS_FNLEN);
	return;
}

void efs_file_delete(unsigned int handle) {
	unsigned int data;
	unsigned int bytecount;
	unsigned int nextfile;
	unsigned int i;
	unsigned char byte;
	if (efs_validate_handle(handle)) return;
	nextfile = handle + EFS_GETWORD(handle) + EFS_HEADERSZ;
	data = EFS_GETWORD(nextfile);
	if (data == 0xFFFF) {
		EFS_PUTWORD(handle,0xFFFF);
		return;
	}
	i = efs_find_endpointer() + 2;
	bytecount = i - handle; 
	for(i=0;i<bytecount;i++) {
		byte = EFS_GETBYTE(nextfile);
		EFS_PUTBYTE(handle,byte);
		nextfile++;
		handle++;
	}
	return;
}


static unsigned int efs_file_rw(unsigned int handle, unsigned char*buf, unsigned int begin, unsigned int len, unsigned char rw) {
	unsigned int filesz;
	if (efs_validate_handle(handle)) return 0;
	filesz = EFS_GETWORD(handle);
	if ((begin + len) > filesz) return 0;
	handle += EFS_HEADERSZ + begin;
	if (rw) EFS_GETBLOCK(buf,handle,len);
	else EFS_PUTBLOCK(buf,handle,len);
	return len;
	}

unsigned int efs_file_read(unsigned int handle, unsigned char*buf, unsigned int begin, unsigned int len) {
	return efs_file_rw(handle,buf,begin,len,1);
}

unsigned int efs_file_write(unsigned int handle, unsigned char*buf, unsigned int begin, unsigned int len) {
	return efs_file_rw(handle,buf,begin,len,0);
}


static unsigned int efs_file_shrink(unsigned int handle, unsigned int newsize, unsigned int oldsize) {
	unsigned int i;
	unsigned int bytecnt;
	unsigned int newnextf;
	unsigned int oldnextf;
	unsigned char byte;
	newnextf = handle + newsize + EFS_HEADERSZ;
	oldnextf = handle + oldsize + EFS_HEADERSZ;
	i = efs_find_endpointer() + 2;
	bytecnt = i - oldnextf; 
	for(i=0;i<bytecnt;i++) {
		byte = EFS_GETBYTE(oldnextf);
		EFS_PUTBYTE(newnextf,byte);
		oldnextf++;
		newnextf++;
	}
	EFS_PUTWORD(handle,newsize);
	return handle;
}
	
static unsigned int efs_file_enlarge(unsigned int handle, unsigned int newsize, unsigned int oldsize) {
	unsigned char* datastore;
	unsigned char name[EFS_FNLEN+1];
	EFS_GETBLOCK(name,(handle+2),EFS_FNLEN);
	datastore = alloca(oldsize);
	if (!datastore) return handle;
	EFS_GETBLOCK(datastore,(handle+EFS_HEADERSZ),oldsize);
	efs_file_delete(handle);
	name[EFS_FNLEN] = 0;
	handle = efs_file_new(name,newsize);
	if (handle == 0xFFFF) handle = efs_file_new(name,oldsize);
	efs_file_write(handle,datastore,0,oldsize);
	return handle;
}


unsigned int efs_file_chsize(unsigned int handle, unsigned int newsz) {
	unsigned int oldsz;
	unsigned int newnextf;
	unsigned int oldnextf;
	unsigned int data;
	if (efs_validate_handle(handle)) return handle;
	oldsz = EFS_GETWORD(handle);
	
	newnextf = handle + newsz + EFS_HEADERSZ;
	oldnextf = handle + oldsz + EFS_HEADERSZ;
	data = EFS_GETWORD(oldnextf);
	if (data == 0xFFFF) {
		if ((newnextf+2) >= NEWEFS_SZ) return handle;
		EFS_PUTWORD(handle,newsz);
		EFS_PUTWORD(newnextf,0xFFFF);
		return handle;
	}
	if (oldsz > newsz) return efs_file_enlarge(handle,newsz,oldsz);
	if (oldsz < newsz) return efs_file_shrink(handle,newsz,oldsz);
	return handle;
}

unsigned int efs_file_getsize(unsigned int handle) {
	if (efs_validate_handle(handle)) return 0xFFFF;
	return EFS_GETWORD(handle);
	}
	
unsigned int efs_file_getnextname(unsigned int handle, unsigned char *buf) {
	unsigned int data;
	if (efs_validate_handle(handle)) return 0xFFFF;
	if (handle == 0xFFFE) handle = EFS_FILEAREA;
	else {
		data = EFS_GETWORD(handle);
		if (data == 0xFFFF) return data;
		handle += data+EFS_HEADERSZ;
	}
	data = EFS_GETWORD(handle);
	if (data == 0xFFFF) return data;
	EFS_GETBLOCK(buf,(handle+2),EFS_FNLEN);
	buf[EFS_FNLEN] = 0;
	return handle;
}

unsigned int efs_get_freespace(void) {
	unsigned int i;
	i = efs_find_endpointer();
	i += 2;
	return NEWEFS_SZ - i;
}

#endif
