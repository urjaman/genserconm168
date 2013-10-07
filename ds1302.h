#ifndef _DS1302_INCLUDED_
#define _DS1302_INCLUDED_


unsigned char ds1302_read(unsigned char addr);
void ds1302_write(unsigned char addr,unsigned char data);
void rtc_init(void);
void rtc_get_time(unsigned char *hour,unsigned char *min,unsigned char *sec);
void rtc_set_time(unsigned char hour,unsigned char min,unsigned char sec);
void rtc_get_date(unsigned char *date,unsigned char *month,unsigned char *year);
void rtc_set_date(unsigned char date,unsigned char month,unsigned char year);
void rtc_ram_write(unsigned char addr, unsigned char data);
unsigned char rtc_ram_read(unsigned char addr);

#endif

