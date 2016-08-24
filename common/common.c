/*
	Vitamin
	Copyright (C) 2016, Team FreeK

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "graphics.h"

void printLayout(char *info, char *title) {
	psvDebugScreenResetMargin();

	psvDebugScreenSetLeftMargin(1);
	psvDebugScreenSetXY(0, 2);

	// Title
	psvDebugScreenSetFgColor(CYAN);
	printf("Vitamin by Team FreeK");
	psvDebugScreenSetFgColor(WHITE);

	// Battery
	char percent[8];
	sprintf(percent, "%d%%", scePowerGetBatteryLifePercent());
	psvDebugScreenSetXY(59 - strlen(percent), psvDebugScreenGetY());
	printf("%s\n", percent);

	printf("----------------------------------------------------------\n\n");

	// Info
	psvDebugScreenSetFgColor(GRAY);
	psvDebugScreenSetLeftMargin(2);
	printf(info);

	// Title
	psvDebugScreenSetLeftMargin(3);
	psvDebugScreenSetFgColor(GREEN);
	printf("\n\n%s\n\n", title);

	int x = psvDebugScreenGetX();
	int y = psvDebugScreenGetY();

	psvDebugScreenSetLeftMargin(1);
	psvDebugScreenSetXY(0, 27);
	psvDebugScreenSetFgColor(WHITE);
	printf("----------------------------------------------------------\n\n");

	psvDebugScreenSetXY(x, y);

	psvDebugScreenSetLeftMargin(3);
	psvDebugScreenSetRightMargin(57);
	psvDebugScreenSetTopMargin(y);
	psvDebugScreenSetBottomMargin(24);
}

int power_tick_thread(SceSize args, void *argp) {
	while (1) {
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_OLED_OFF);
		sceKernelDelayThread(10 * 1000 * 1000);
	}

	return 0;
}

void initPowerTickThread() {
	SceUID thid = sceKernelCreateThread("power_tick_thread", power_tick_thread, 0x10000100, 0x40000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);
}