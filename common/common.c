/*
	Vitamin
	Copyright (C) 2016, Team FreeK (TheFloW, Major Tom, mr. gas)

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

#include <psp2/appmgr.h>
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "graphics.h"

void SetLayoutMargin(int y) {
	psvDebugScreenSetLeftMargin(3);
	psvDebugScreenSetRightMargin(57);
	psvDebugScreenSetTopMargin(y);
	psvDebugScreenSetBottomMargin(24);
}

int printLayout(char *info, char *title) {
	psvDebugScreenResetMargin();

	// Title
	psvDebugScreenSetLeftMargin(1);
	psvDebugScreenSetXY(0, 2);
	psvDebugScreenSetFgColor(CYAN);
	printf("Vitamin v%d.%d by TheFloW, Major Tom, mr. gas", VITAMIN_VERSION_MAJOR, VITAMIN_VERSION_MINOR);

	// Battery
	char percent[8];
	sprintf(percent, "%d%%", scePowerGetBatteryLifePercent());
	psvDebugScreenSetXY(59 - strlen(percent), psvDebugScreenGetY());
	psvDebugScreenSetFgColor(DARKGREEN);
	printf("%s\n", percent);

	psvDebugScreenSetFgColor(WHITE);
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

	SetLayoutMargin(y);

	return y;
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

int launchAppByUriExit(char *titleid) {
	char uri[32];
	sprintf(uri, "psgm:play?titleid=%s", titleid);
/*
	sceKernelDelayThread(5000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelDelayThread(5000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
*/
	do {
		sceKernelDelayThread(5000);
		sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	} while (!sceAppMgrIsOtherAppPresent());

	sceKernelExitProcess(0);

	return 0;
}