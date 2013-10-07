#include "lib.h"
#include "ds1302.h"

void rtc_init(void)
{
ds1302_write(0x8e,0);
ds1302_write(0x90,0xA7);
}

void rtc_get_time(unsigned char *hour,unsigned char *min,unsigned char *sec)
{
*hour=bcd2bin(ds1302_read(0x85));
*min=bcd2bin(ds1302_read(0x83));
*sec=bcd2bin(ds1302_read(0x81));
}

void rtc_set_time(unsigned char hour,unsigned char min,unsigned char sec)
{
ds1302_write(0x84,bin2bcd(hour));
ds1302_write(0x82,bin2bcd(min));
ds1302_write(0x80,bin2bcd(sec));
}

void rtc_get_date(unsigned char *date,unsigned char *month,unsigned char *year)
{
*date=bcd2bin(ds1302_read(0x87));
*month=bcd2bin(ds1302_read(0x89));
*year=bcd2bin(ds1302_read(0x8d));
}

void rtc_set_date(unsigned char date,unsigned char month,unsigned char year)
{
ds1302_write(0x86,bin2bcd(date));
ds1302_write(0x88,bin2bcd(month));
ds1302_write(0x8c,bin2bcd(year));
}

void rtc_ram_write(unsigned char addr, unsigned char data) {
	addr = ((addr<<1)|0xC0);
	if (addr == 0xFE) return;
	ds1302_write(addr,data);
}

unsigned char rtc_ram_read(unsigned char addr) {
	addr = ((addr<<1)|0xC1);
	if (addr == 0xFF) return 0xFF;
	return ds1302_read(addr);
}
