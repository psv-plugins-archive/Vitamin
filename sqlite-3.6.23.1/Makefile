TARGET_LIB = libsqlite3.a
OBJS       = sqlite3.o

PREFIX  ?= ${VITASDK}/arm-vita-eabi
CC      = arm-vita-eabi-gcc
AR      = arm-vita-eabi-ar
CFLAGS  = -Wl,-q -Wall -O3 -DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0
ASFLAGS = $(CFLAGS)

all: $(TARGET_LIB)

debug: all

$(TARGET_LIB): $(SHADERS) $(OBJS)
	$(AR) -rc $@ $^

clean:
	rm -rf $(TARGET_LIB) $(OBJS)