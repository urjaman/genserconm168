#define DS1302_DELAY 5
#define DS1302_LDELAY 0xD
#define __ds1302_port 0x08
#define __ds1302_ce 2
#define __ds1302_io 1
#define __ds1302_sclk 0

_ds1302_ldelay:
	ldi r18, DS1302_LDELAY
	rjmp _ds1302_delay0
_ds1302_delay:
	ldi	r18, DS1302_DELAY
_ds1302_delay0:
	dec	r18
	brne	_ds1302_delay0
	ret


.global	ds1302_read
	.type	ds1302_read, @function
ds1302_read:
	ori	r24, 1
	rcall __ds1302_write0
	cbi  __ds1302_port,__ds1302_io
	cbi  __ds1302_port-1,__ds1302_io
	ldi  r26,8
__ds1302_read0:
	clc
	sbic __ds1302_port-2,__ds1302_io
	sec
	ror  r24
	sbi  __ds1302_port,__ds1302_sclk
	rcall _ds1302_delay
	cbi  __ds1302_port,__ds1302_sclk
	rcall _ds1302_delay
	dec  r26
	brne __ds1302_read0

	clr r25
__ds1302_rst0:
	cbi  __ds1302_port,__ds1302_ce
	rcall _ds1302_ldelay
	ret

.global	ds1302_write
	.type	ds1302_write, @function
ds1302_write:
	andi	r24, 0xFE
	rcall __ds1302_write0
	mov	r24, r22
	rcall __ds1302_write1
	rjmp __ds1302_rst0

__ds1302_write0:
	sbi  __ds1302_port-1,__ds1302_sclk
	sbi  __ds1302_port-1,__ds1302_io
	sbi  __ds1302_port-1,__ds1302_ce
	sbi  __ds1302_port,__ds1302_ce
	rcall _ds1302_ldelay

__ds1302_write1:
	ldi  r26,8
__ds1302_write2:
	ror  r24
	cbi  __ds1302_port,__ds1302_io
	brcc __ds1302_write3
	sbi  __ds1302_port,__ds1302_io
__ds1302_write3:
	nop
	nop
	nop
	nop
	sbi  __ds1302_port,__ds1302_sclk
	rcall _ds1302_delay	
	cbi __ds1302_port,__ds1302_sclk
	rcall _ds1302_delay
	dec  r26
	brne __ds1302_write2
	ret
