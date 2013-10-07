/* GENERIC DEFINITIONS FOR PROJECT */
#define F_CPU 18432000
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#define MAXTOKENS 16
extern unsigned char token_count;
extern unsigned char* tokenptrs[];


//#define USE_EFS
//#define HAVE_GETKHZ
