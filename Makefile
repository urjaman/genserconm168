# AVR-GCC Makefile
PROJECT=testproj
SOURCES=main.c uart.c console.c lib.c appdb.c commands.c ds1302c.c ds1302.S efs.c flash.c
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atmega168
AVRBINDIR=~/avr-tools/bin
AVRDUDECMD=avrdude -p m168 -c dt006 -E noreset
CFLAGS=-mmcu=$(MMCU) -Os -mcall-prologues -fno-inline-small-functions -fno-tree-scev-cprop -frename-registers -g -Werror -Wall -W -pipe -combine -fwhole-program
 
$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)/$(OBJCOPY) -j .text -O ihex $(PROJECT).out $(PROJECT).hex
 
$(PROJECT).out: $(SOURCES)
	$(AVRBINDIR)/$(CC) $(CFLAGS) -I./ -o $(PROJECT).out $(SOURCES)
 
program: $(PROJECT).hex
	$(AVRBINDIR)/$(AVRDUDECMD) -U flash:w:$(PROJECT).hex

clean:
	rm -f $(PROJECT).out
	rm -f $(PROJECT).hex

backup:
	$(AVRBINDIR)/$(AVRDUDECMD) -U flash:r:backup.bin:r
