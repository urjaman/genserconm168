struct command_t {
	PGM_P name;
	void(*function)(void);
};

extern struct command_t appdb[] PROGMEM;

void *find_appdb(unsigned char* cmd);
void echo_cmd(void);
void time_cmd(void);
void date_cmd(void);
void rtcram_cmd(void);
void efs_wr_cmd(void);
void efs_ls_cmd(void);
void efs_cat_cmd(void);
void efs_rm_cmd(void);
void efs_mv_cmd(void);
void efs_cp_cmd(void);
void help_cmd(void);
void flash_readsect_cmd(void);
void flasher_interface_cmd(void);
void flash_readchip_cmd(void);
void flash_idchip_cmd(void);
void calc_cmd(void);
void getkhz_cmd(void);

