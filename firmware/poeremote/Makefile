CONTIKI = ../contiki-xmega
#CONTIKI = ../contiki-xmega
#CONTIKI = ../contiki-2.5.rc1
ifndef TARGET
TARGET=avr-poe
endif

#APPS=lcd-test

CONTIKIAVR=$(CONTIKI)/cpu/avr


all:
	make TARGET=avr-poe -f Makefile.poewall poewall.elf
	avr-objcopy -O ihex -R .signature -R .fuse -R .eeprom poewall.elf poewall.hex
		avr-size -C --mcu=atxmega32a4 poewall.elf

# clean:
# 	make -f Makefile.poewall clean
# 		rm -rf obj_avr-poe
# 		rm symbols.c symbols.h poewall.elf poewall.hex

install: all
	kdesudo "avrdude -v -v -P usb -c avrispmkII -p x32a4 -U flash:w:poewall.hex"


clean:
	rm -rf ./*.o
	rm -rf ./obj_avr-poe/*
	rm -rf ./*.elf

include $(CONTIKIAVR)/Makefile.avr_poe
#include $(CONTIKIAVR)/Makefile.avr
#include $(CONTIKI)/Makefile.include

