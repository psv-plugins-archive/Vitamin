TITLE_ID = VITAMIN00
TARGET   = injection
OBJS     = main.o ../common/utils.o ../common/graphics.o ../common/font.o vita_sqlite.o ../minizip/makezip.o ../minizip/zip.o ../minizip/ioapi.o

LIBS     = -lSceAppMgr_stub -lSceCtrl_stub -lSceDisplay_stub -lSceKernel_stub -lz libsqlite3.a -lScePromoterUtil_stub

PREFIX   = arm-vita-eabi
CC       = $(PREFIX)-gcc
CFLAGS   = -Wl,-q -Wall -O3
ASFLAGS  = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "Vitamin" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
		--add pkg/sce_sys/icon0.png=sce_sys/icon0.png \
		--add pkg/sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
		--add pkg/sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
		--add pkg/sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
	$(TARGET).vpk

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
