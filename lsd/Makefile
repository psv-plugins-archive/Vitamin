TARGET  = lsd
OBJS    = main.o payload.o ../common/utils.o ../common/graphics.o ../common/font.o

LIBS    = -lSceAppMgr_stub -lSceCtrl_stub -lSceDisplay_stub -lSceKernel_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: $(TARGET).suprx

%.suprx: %.velf
	vita-make-fself $< $@

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).suprx $(TARGET).velf $(TARGET).elf $(OBJS)

send: $(TARGET).suprx
	curl -T $(TARGET).suprx ftp://$(PSVITAIP):1337/ux0:/DUMP00000/sce_module/libc_.suprx
	@echo "Sent."