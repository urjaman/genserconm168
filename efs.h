/* EEPROM FILESYSTEM */
/* FILESYSTEM DATA REPRESENTATION */
/* <FILE0 HEADER> <FILE0 DATA> ... <FILEn HEADER><FILEn DATA> <END/FREE MARKER> */


/* FILEn HEADER */
/* SIZE	OFFSET	MEANING */
/* 2	0	FILE SIZE (BYTES) -- 0xFFFF == NO FILE/END MARKER (GT FS SIZE == INVALID FS) */
/* 8	2	FILE NAME (NULL PADDED (limit to 7-bit ASCII?)) */

#define NEWEFS_SZ 512 /* IN CASE A NEW FS IS CREATED, ITS SIZE */
#define EFSOFFSET 0
/* THE FS CAN BE MOVED ABOUT IN EEPROM IF NEEDED */
#define EFS_FNLEN 8
/* FS API */
/* "file handle" is an offset (which certain functions may change, so they return new one) */
/* file's can be created, "opened", deleted, read from, written to, size changed (MUST DO) */
/* and size gotten, and root directory iterated */
/* - 0xFFFE handle in efs_getnextname means find first handle */
/* - a (-1) handle means invalid file */

unsigned int efs_file_open(unsigned char *fn);
unsigned int efs_file_new(unsigned char *fn, unsigned int size);
void efs_file_chname(unsigned int handle, unsigned char *fn);
void efs_file_delete(unsigned int handle);
unsigned int efs_file_read(unsigned int handle, unsigned char*buf, unsigned int begin, unsigned int len);
unsigned int efs_file_write(unsigned int handle, unsigned char*buf, unsigned int begin, unsigned int len);
unsigned int efs_file_chsize(unsigned int handle, unsigned int newsize);
unsigned int efs_file_getsize(unsigned int handle);
unsigned int efs_file_getnextname(unsigned int handle, unsigned char *buf);
unsigned int efs_get_freespace(void);
void efs_format(void);
