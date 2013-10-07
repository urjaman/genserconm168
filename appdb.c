#include "main.h"
#include "console.h"
#include "appdb.h"



unsigned char echostr[] PROGMEM = "ECHO";
unsigned char timestr[] PROGMEM = "TIME";
unsigned char datestr[] PROGMEM = "DATE";
unsigned char rtcramstr[] PROGMEM = "RTCRAM";
unsigned char readsectstr[] PROGMEM = "READSECT";
unsigned char flashistr[] PROGMEM = "FLASHI";
unsigned char flashrcstr[] PROGMEM = "READCHIP";
unsigned char flashidstr[] PROGMEM = "IDCHIP";
unsigned char calcstr[] PROGMEM = "CALC";
#ifdef HAVE_GETKHZ
unsigned char getkhzstr[] PROGMEM = "GETKHZ";
#endif

#ifdef USE_EFS
unsigned char efslsstr[] PROGMEM = "LS";
unsigned char efswrstr[] PROGMEM = "WR";
unsigned char efscatstr[] PROGMEM = "CAT";
unsigned char efsrmstr[] PROGMEM = "RM";
unsigned char efsmvstr[] PROGMEM = "MV";
unsigned char efscpstr[] PROGMEM = "CP";
#endif

unsigned char helpstr[] PROGMEM = "?";

struct command_t appdb[] PROGMEM = {
	{(PGM_P)echostr, &(echo_cmd)},
	{(PGM_P)timestr, &(time_cmd)},
	{(PGM_P)datestr, &(date_cmd)},
	{(PGM_P)rtcramstr, &(rtcram_cmd)},
	{(PGM_P)readsectstr, &(flash_readsect_cmd)},
	{(PGM_P)flashistr, &(flasher_interface_cmd)},
	{(PGM_P)flashrcstr, &(flash_readchip_cmd)},
	{(PGM_P)flashidstr, &(flash_idchip_cmd)},
	{(PGM_P)calcstr, &(calc_cmd)},
#ifdef HAVE_GETKHZ
	{(PGM_P)getkhzstr, &(getkhz_cmd)},
#endif
	
#ifdef USE_EFS
	{(PGM_P)efslsstr, &(efs_ls_cmd)},
	{(PGM_P)efswrstr, &(efs_wr_cmd)},
	{(PGM_P)efscatstr, &(efs_cat_cmd)},
	{(PGM_P)efsrmstr, &(efs_rm_cmd)},
	{(PGM_P)efsmvstr, &(efs_mv_cmd)},
	{(PGM_P)efscpstr, &(efs_cp_cmd)},
#endif
	
	{(PGM_P)helpstr, &(help_cmd)},
	{NULL,NULL}
};

void invalid_command(void) {
	sendstr(tokenptrs[0]);
	sendstr_P(PSTR(": not found"));
	}



void *find_appdb(unsigned char* cmd) {
	unsigned char i;
	struct command_t * ctptr;
	PGM_P name;
	void* fp;
	for(i=0;;i++) {
		ctptr = &(appdb[i]);
		name = (PGM_P)pgm_read_word(&(ctptr->name));
		fp = (void*)pgm_read_word(&(ctptr->function));
		if (!name) break;
		if (strcmp_P((char*)cmd,name) == 0) {
			return fp;
			}
	}
	return &(invalid_command);
}


