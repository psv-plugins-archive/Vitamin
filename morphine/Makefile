TITLE_ID = MORPHINE0
TARGET   = Morphine
OBJS     = main.o ../common/common.o ../common/utils.o ../common/graphics.o ../common/font.o ../minizip/makezip.o ../minizip/zip.o ../minizip/ioapi.o

LIBS     = -lSceAppMgr_stub -lSceCtrl_stub -lSceDisplay_stub -lSceKernel_stub -lScePower_stub -lz

PREFIX   = arm-vita-eabi
CC       = $(PREFIX)-gcc
CFLAGS   = -Wl,-q -Wall -O3
ASFLAGS  = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin $@

eboot.bin: $(TARGET).velf
	vita-make-fself $< $@

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).vpk $(TARGET).velf $(TARGET).elf $(OBJS) \
		eboot.bin param.sfo

vpksend: $(TARGET).vpk
	curl -T $(TARGET).vpk ftp://$(PSVITAIP):1337/ux0:/
	@echo "Sent."

send: eboot.bin
	curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/$(TITLE_ID)/
	@echo "Sent."