all: clean _steroid _lsd _morphine _injection

send: all
	make -C injection send
	make -C injection clean

clean:
	-rm "morphine/steroid.h"
	-rm "morphine/lsd.h"
	-rm "injection/eboot_bin.h"
	-rm "injection/param_sfo.h"

_steroid:
	make -C steroid clean
	make -C steroid
	bin2c steroid/steroid.suprx morphine/steroid.h steroid
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
	bin2c morphine/param.sfo injection/param_sfo.h param_sfo
	make -C morphine clean
	
_injection:
	make -C injection clean
	make -C injection