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

#include <psp2/appmgr.h>
#include <psp2/display.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#include "../common/utils.h"
#include "../common/graphics.h"
#include "../common/common.h"

#include "../minizip/makezip.h"

#include "lsd.h"

static char titleid[12];
static int layout_y = 0;

int debugPrintf(char *text, ...) {
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	SceUID fd = sceIoOpen("ux0:morphine_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}

	return 0;
}

int ignoreHandler(char *path) {
	char *ext  = strrchr(path, '.');
	if (ext && strcmp(ext, ".self") == 0) {
		return 1;
	}

	if (strstr(path, "eboot.bin") || strstr(path, "param.sfo") || strstr(path, "sce_module") || strstr(path, "clearsign") || strstr(path, "keystone")) {
		return 1;
	}

	return 0;
}

void restoreSavedata() {
	removePath("ux0:user/00/savedata_old");
	if (sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_old") >= 0) {
		if (sceIoRename("ux0:user/00/savedata_org", "ux0:user/00/savedata") < 0) {
			sceIoRename("ux0:user/00/savedata_old", "ux0:user/00/savedata");
		}
	}
}

void relaunchGame() {
	// Write relaunch titleid
	WriteFile("ux0:pspemu/Vitamin/relaunch.bin", titleid, strlen(titleid) + 1);

	// Launch Vitamin app to relaunch game
	launchAppByUriExit("VITAMIN00");
}

int patchSfo(char *app_path, char *zip_path) {
	char param_sfo_path[128];
	uint32_t attribute = 0;
	void *buffer = NULL;

	// Read param.sfo
	sprintf(param_sfo_path, "%s/sce_sys/param.sfo", app_path);

	int size = allocateReadFile(param_sfo_path, &buffer);
	if (size <= 0)
		return size;

	// Get and patch attribute
	SfoHeader *header = (SfoHeader *)buffer;
	SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp(buffer + header->keyofs + entries[i].nameofs, "ATTRIBUTE") == 0) {
			attribute = *(int *)(buffer + header->valofs + entries[i].dataofs);
			debugPrintf("original attribute = 0x%08X\n", attribute);
			attribute &= 0xFFFFFBFB;
			*(uint32_t *)(buffer + header->valofs + entries[i].dataofs) = attribute;
			break;
		}
	}

	debugPrintf("new attribute = 0x%08X\n", attribute);

	sceIoMkdir("ux0:pspemu/Vitamin/sce_sys", 0777);
	WriteFile("ux0:pspemu/Vitamin/sce_sys/param.sfo", buffer, size);

	// Free buffer
	free(buffer);

	return 0;
}

int copyExecutables(char *src_path, char *dst_path) {
	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (SCE_S_ISDIR(dir.d_stat.st_mode))
					continue;

				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *ext = strrchr(dir.d_name, '.');
				if (!ext)
					continue;

				if (strcmp(ext, ".self") != 0 && strcmp(dir.d_name, "eboot.bin") != 0) {
					continue;
				}

				char *new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
				snprintf(new_src_path, MAX_PATH_LENGTH, "%s/%s", src_path, dir.d_name);

				char *new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
				snprintf(new_dst_path, MAX_PATH_LENGTH, "%s/%s", dst_path, dir.d_name);

				int ret = copyFile(new_src_path, new_dst_path);
				debugPrintf("Copy %s to %s\n", new_src_path, new_dst_path);

				free(new_dst_path);
				free(new_src_path);

				if (ret < 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	}

	return 0;
}

static uint32_t previous_value = 0;
static SceUInt64 cur_micros = 0, delta_micros = 0, last_micros = 0;
static double kbs = 0;

void SetProgress(uint64_t value, uint64_t max) {
	int x = psvDebugScreenGetX();
	int y = psvDebugScreenGetY();
	psvDebugScreenResetMargin();

	double progress = (double)((double)value / (double)max);
	double progress_percent = (double)(100.0f * progress);
	double progress_bar = (double)(51.0f * progress);

	// KB/s
	cur_micros = sceKernelGetProcessTimeWide();
	if (cur_micros >= (last_micros + 1000000)) {
		delta_micros = cur_micros - last_micros;
		last_micros = cur_micros;
		kbs = (double)(value - previous_value) / 1024.0f;
		previous_value = value;
	}

	psvDebugScreenSetFgColor(PURPLE);
	psvDebugScreenSetXY(2, 29);
	printf("> Dumping...%d%% (%.2f KB/s)", (int)progress_percent, kbs);

	psvDebugScreenSetFgColor(WHITE);
	psvDebugScreenSetXY(4, 30);
	printf("[");

	int i;
	for (i = 0; i < (int)progress_bar; i++) {
		printf("*");
	}

	psvDebugScreenSetXY(55, 30);
	printf("]");

	SetLayoutMargin(layout_y);
	psvDebugScreenSetXY(x, y);
}

int main(int argc, char *argv[]) {
	char path[128], dst_path[128], app_path[128], tmp_path[128];

	// Set CPU clock to 444mhz
	scePowerSetArmClockFrequency(444);

	// Init power tick thread
	initPowerTickThread();

	// Get title id
	memset(titleid, 0, sizeof(titleid));
	sceAppMgrAppParamGetString(sceKernelGetProcessId(), SCE_APPMGR_APP_PARAM_TITLE_ID, titleid, sizeof(titleid));

	// Read mode
	int mode = 0;
	ReadFile("app0:mode.bin", &mode, sizeof(int));

	// Read game info
	GameInfo game_info;
	ReadFile("app0:info.bin", &game_info, sizeof(GameInfo));

	// Init screen
	psvDebugScreenInit();
	psvDebugScreenClear(DARKBLUE);
	psvDebugScreenSetBgColor(DARKBLUE);

	// Layout
	char *version = mode == MODE_UPDATE ? game_info.version_update : game_info.version_game;
	char game_info_string[512];
	sprintf(game_info_string, "Name   : %s\n"
							  "Game ID: %s\n"
							  "Version: %s\n",
							  game_info.name,
							  game_info.titleid,
							  (version[0] == '0') ? (version + 1) : version);
	layout_y = printLayout(game_info_string, mode == MODE_UPDATE ? "Dumping update files" : "Dumping full game");

	// Dump process
	sprintf(app_path, "%s:app/%s", game_info.is_cartridge ? "gro0" : "ux0", titleid);
	// ux0:app/ABCD12345

	if (mode == MODE_UPDATE) {
		// Dump decrypted files
		sprintf(dst_path, "ux0:Vitamin/%s_UPDATE_%s.ZIP", titleid, game_info.version_update);
		sceIoRemove(dst_path);

		uint64_t value = 0;
		makeZip(dst_path, app_path, game_info.is_cartridge ? 19 : 18, 0, &value, game_info.size, SetProgress, ignoreHandler);
		SetProgress(game_info.size, game_info.size);
		sceKernelDelayThread(1 * 1000 * 1000);
		psvDebugScreenClearLine(29, DARKBLUE);
		psvDebugScreenClearLine(30, DARKBLUE);

		// Write patched SFO
		patchSfo(app_path, dst_path);

		// Copy executables to temporary directory
		printf("Copying executable files for decryption...");
		copyExecutables(app_path, "ux0:pspemu/Vitamin_exec");
		printf("OK\n");

		// Destory all other apps (close manual to unlock files)
		sceAppMgrDestroyOtherApp();
		sceKernelDelayThread(1 * 1000 * 1000);

		// eboot.bin patch path
		sprintf(dst_path, "ux0:patch/%s/eboot.bin", titleid);

		// Delete *this* eboot.bin
		sceIoRemove(dst_path);

		// Backup patch in app folder
		sprintf(tmp_path, "ux0:app/%s_patch", titleid);
		sceIoRename(app_path, tmp_path);

		// Restore original app folder
		sprintf(tmp_path, "ux0:app/%s_org", titleid);
		sceIoRename(tmp_path, app_path);
	} else {
		// Dump decrypted files
		sprintf(dst_path, "ux0:Vitamin/%s_FULLGAME_%s.VPK", titleid, game_info.version_game);
		sceIoRemove(dst_path);

		uint64_t value = 0;
		makeZip(dst_path, app_path, game_info.is_cartridge ? 19 : 18, 0, &value, game_info.size, SetProgress, ignoreHandler);
		SetProgress(game_info.size, game_info.size);
		sceKernelDelayThread(1 * 1000 * 1000);
		psvDebugScreenClearLine(29, DARKBLUE);
		psvDebugScreenClearLine(30, DARKBLUE);

		// Write patched SFO
		patchSfo(app_path, dst_path);

		// Copy executables to temporary directory
		printf("Copying executable files for decryption...");
		copyExecutables(app_path, "ux0:pspemu/Vitamin_exec");
		printf("OK\n");

		// Destory all other apps (close manual to unlock files)
		sceAppMgrDestroyOtherApp();
		sceKernelDelayThread(1 * 1000 * 1000);

		// Delete *this* eboot.bin
		sprintf(path, "ux0:patch/%s/eboot.bin", titleid);
		sceIoRemove(path);
	}

	// Restore original savedata
	restoreSavedata();

	// Write lsd module
	sprintf(path, "ux0:patch/%s/sce_module/libc.suprx", titleid);
	WriteFile(path, lsd, size_lsd);

	// Relaunch game
	relaunchGame();

	return 0;
}
