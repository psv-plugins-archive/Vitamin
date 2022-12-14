all: clean _steroid _lsd _morphine _injection

send:
	make -C injection send
	make -C injection clean

clean:
	-rm "morphine/lsd.h"
	-rm "injection/steroid.h"
	-rm "injection/eboot_bin.h"
	make -C injection clean
	make -C morphine clean
	make -C lsd clean
	make -C steroid clean

_steroid:
	make -C steroid clean
	make -C steroid
	bin2c steroid/steroid.suprx injection/steroid.h steroid
	make -C steroid clean

_lsd:
	make -C lsd clean
	make -C lsd
	bin2c lsd/lsd.suprx morphine/lsd.h lsd
	make -C lsd clean

_morphine:
	make -C morphine clean
	make -C morphine
	bin2c morphine/eboot.bin injection/eboot_bin.h eboot_bin
	make -C morphine clean

_injection:
	make -C injection clean
	make -C injection
