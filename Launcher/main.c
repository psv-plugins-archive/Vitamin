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
#include <psp2/ctrl.h>
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

#include <sqlite3.h>

#include "graphics.h"

#include "self.h"
#include "elf.h"

int sceKernelGetProcessId();

#define SCE_APPMGR_APP_PARAM_CONTENT_ID 6
#define SCE_APPMGR_APP_PARAM_CATEGORY 8
#define SCE_APPMGR_APP_PARAM_TITLE_ID 12

#define printf psvDebugScreenPrintf

#define MAX_MODULES 128

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define PROGRAM_HEADER_NUM 2

#define APP0_EBOOT_BIN "app0:eboot.bin"

static char pspemu_path[16];
static char contentid[48];
static char titleid[12];

int debugPrintf(char *text, ...) {
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	char path[128];
	sprintf(path, "%s/log.txt", pspemu_path);
	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}

	return 0;
}

int ReadFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}

int WriteFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}

#define MAX_PATH_LENGTH 1024
#define TRANSFER_SIZE 64 * 1024
#define SCE_ERROR_ERRNO_EEXIST 0x80010011

int copyFile(char *src_path, char *dst_path) {
	char *file_path = (char *)(strstr(src_path, titleid) + 9);

	if (strstr(file_path, "_updt") > 0)
		file_path += 5;

	printf("Dumping %s...", file_path);

	SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
	if (fdsrc < 0) {
		printf("Error 0x%08X\n", fdsrc);
		return fdsrc;
	}

	SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fddst < 0) {
		printf("Error 0x%08X\n", fddst);
		sceIoClose(fdsrc);
		return fddst;
	}

	void *buf = malloc(TRANSFER_SIZE);

	int read;
	while ((read = sceIoRead(fdsrc, buf, TRANSFER_SIZE)) > 0) {
		sceIoWrite(fddst, buf, read);
	}

	free(buf);

	sceIoClose(fddst);
	sceIoClose(fdsrc);

	printf("OK\n");

	return 0;
}

int removePath(char *path) {
	printf("Deleting %s...\n", path);

	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				snprintf(new_path, MAX_PATH_LENGTH, "%s/%s", path, dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					int ret = removePath(new_path);
					if (ret <= 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}
				} else {
					int ret = sceIoRemove(new_path);
					if (ret < 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}
				}

				free(new_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);

		int ret = sceIoRmdir(path);
		if (ret < 0)
			return ret;
	} else {
		int ret = sceIoRemove(path);
		if (ret < 0)
			return ret;
	}

	return 1;
}

int copyPath(char *src_path, char *dst_path) {
	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		int ret = sceIoMkdir(dst_path, 0777);
		if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
			sceIoDclose(dfd);
			return ret;
		}

		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
				snprintf(new_src_path, MAX_PATH_LENGTH, "%s/%s", src_path, dir.d_name);

				char *new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
				snprintf(new_dst_path, MAX_PATH_LENGTH, "%s/%s", dst_path, dir.d_name);

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					ret = copyPath(new_src_path, new_dst_path);
				} else {
					ret = copyFile(new_src_path, new_dst_path);
				}

				free(new_dst_path);
				free(new_src_path);

				if (ret < 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else {
		return copyFile(src_path, dst_path);
	}

	return 0;
}

void waitForUser() {
	uint32_t current_buttons = 0, pressed_buttons = 0;
	while (1) {
		SceCtrlData pad;
		memset(&pad, 0, sizeof(SceCtrlData));
		sceCtrlPeekBufferPositive(0, &pad, 1);

		pressed_buttons = pad.buttons & ~current_buttons;
		current_buttons = pad.buttons;

		if (pressed_buttons & SCE_CTRL_CROSS) {
			break;
		}
		sceDisplayWaitVblankStart();
	}
}

int isCartridge() {
	char path[128];
	sprintf(path, "gro0:app/%s/sce_sys/param.sfo", titleid);

	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;

	sceIoClose(fd);
	return 1;
}

int isDumpStage() {
	SceUID fd = sceIoOpen("app0:eboot.bin", SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;

	sceIoClose(fd);
	return 1;
}

int dumpGame() {
	char path[128];

	// Open manual to decrypt files (locks files at the same time)
	printf("The application will now open the manual of your game in another application.\nMinimize the new application and go back to this game.\n");
	psvDebugScreenSetFgColor(COLOR_RED);
	printf("DO NOT CLOSE THE NEWLY OPENED APPLICATION.\n\n");
	sceKernelDelayThread(3 * 1000 * 1000);

	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("Press X to continue.\n\n");
	waitForUser();

	if (isCartridge())
		sprintf(path, "gro0:app/%s", titleid);
	else
		sprintf(path, "ux0:app/%s", titleid);

	// Backup savedata and let the application create a new savegame with its new encryption key
	sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_bck");

	// Open the manual
	sceAppMgrLaunchAppByUri(0xFFFFF, path);
	sceKernelExitProcess(0);

	return 0;
}

int dumpUpdate() {
	char app_path[128], patch_path[128], tmp_path[128];

	// Create an empty file to ensure this application  is running in update dumping mode
	SceUID fd = sceIoOpen("ux0:pspemu/updt.bin", SCE_O_CREAT | SCE_O_WRONLY, 0777);
	sceIoClose(fd);

	// Backup original application (for digital games only)
	sprintf(app_path, "ux0:app/%s", titleid);
	sprintf(tmp_path, "ux0:app/%s_org", titleid);
	sceIoRename(app_path, tmp_path);

	// Copy patch files into ux0:app
	sprintf(patch_path, "ux0:patch/%s_updt", titleid);
	printf("Copying patch files to ux0:app...\n");
	copyPath(patch_path, app_path);

	// Open manual to decrypt files (locks files at the same time)
	printf("The application will now open the manual of your game in another application.\nMinimize the new application and go back to this game.\n");
	psvDebugScreenSetFgColor(COLOR_RED);
	printf("DO NOT CLOSE THE NEWLY OPENED APPLICATION.\n\n");
	sceKernelDelayThread(3 * 1000 * 1000);

	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("Press X to continue.\n\n");
	waitForUser();

	sceAppMgrLaunchAppByUri(0xFFFFF, app_path);
	sceKernelDelayThread(1 * 1000 * 1000); // Let the shell open the manual

	// Backup savedata and let the application create a new savegame with its new encryption key
	sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_bck");

	// Restart the game
	sprintf(tmp_path, "psgm:play?titleid=%s", titleid);
	sceAppMgrLaunchAppByUri(0xFFFFF, tmp_path);
	sceKernelExitProcess(0);

	return 0;
}

void printInfo() {
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("\nVita Game Dumper\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("by Team FreeK\n\n");
}

int main(int argc, char *argv[]) {
	// Init screen
	psvDebugScreenInit();

	// Get pspemu path
	memset(pspemu_path, 0, sizeof(pspemu_path));
	sceAppMgrPspSaveDataRootMount(pspemu_path);

	// Get content id
	memset(contentid, 0, sizeof(contentid));
	sceAppMgrAppParamGetString(sceKernelGetProcessId(), SCE_APPMGR_APP_PARAM_CONTENT_ID, contentid, sizeof(contentid));

	// Get title id
	memset(titleid, 0, sizeof(titleid));
	sceAppMgrAppParamGetString(sceKernelGetProcessId(), SCE_APPMGR_APP_PARAM_TITLE_ID, titleid, sizeof(titleid));

	if (isDumpStage()) {
		int update_mode = 0;
		SceUID fd;
		char path[128], dst_path[128], tmp_path[128];

		if ((fd = sceIoOpen("ux0:pspemu/updt.bin", SCE_O_RDONLY, 0)) > 0) {
			sceIoClose(fd);
			update_mode = 1;
		}

		if (update_mode == 1) {
			// Dump decrypted files
			sprintf(dst_path, "ux0:pspemu/%s_updt", titleid);
			sprintf(path, "ux0:app/%s", titleid);
			copyPath(path, dst_path);

			// Close manual to unlock files
			printf("\nPlease close the manual application, then go back to this game.\n\n");
			sceKernelDelayThread(3 * 1000 * 1000);
			sceAppMgrLaunchAppByUri(0xFFFF, path);
			sceKernelDelayThread(1 * 1000 * 1000);

			// Copy decrypted eboot.bin to ux0:patch for next stage
			sprintf(path, "ux0:pspemu/%s_updt/eboot.bin", titleid);
			sprintf(dst_path, "ux0:patch/%s/eboot.bin", titleid);
			copyPath(path, dst_path);

			// Restore original app folder
			sprintf(path, "ux0:app/%s", titleid);
			removePath(path);
			sprintf(tmp_path, "ux0:app/%s_org", titleid);
			sceIoRename(tmp_path, path);

			// Restore original savedata
			removePath("ux0:user/00/savedata_old"); // Delete any previous dummy savedata folder
			sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_old");
			sceIoRename("ux0:user/00/savedata_bck", "ux0:user/00/savedata");
		} else { // update_mode == 0
			// Dump decrypted files
			sprintf(dst_path, "ux0:pspemu/%s", titleid);
			if (isCartridge())
				sprintf(path, "gro0:app/%s", titleid);
			else
				sprintf(path, "ux0:app/%s", titleid);
			copyPath(path, dst_path);

			// Close manual to unlock files
			printf("\nPlease close the manual application, then go back to this game.\n\n");
			sceKernelDelayThread(3 * 1000 * 1000);
			sceAppMgrLaunchAppByUri(0xFFFF, path);
			sceKernelDelayThread(1 * 1000 * 1000);

			// Delete *this* eboot.bin
			sprintf(path, "ux0:patch/%s/eboot.bin", titleid);
			sceIoRemove(path);

			// Restore original savedata
			removePath("ux0:user/00/savedata_old");
			sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_old");
			sceIoRename("ux0:user/00/savedata_bck", "ux0:user/00/savedata");
		}

		// Rename libc_.suprx to libc.suprx to allow module hijacking
		sprintf(path, "ux0:patch/%s/sce_module/libc_.suprx", titleid);
		sprintf(tmp_path, "ux0:patch/%s/sce_module/libc.suprx", titleid);
		sceIoRename(path, tmp_path);

		printf("The game will exit one last time. Start it again to decrypt the eboot.bin.\n");
		sceKernelDelayThread(4 * 1000 * 1000);
		sceKernelExitProcess(0);
	}

	uint32_t current_buttons = 0, pressed_buttons = 0;
	int sel = 0;

	while (1) {
		psvDebugScreenSetXY(0, 0);

		printInfo();

		printf("Game ID: ");
		psvDebugScreenSetFgColor(COLOR_GREEN);
		printf("%s\n", titleid);

		psvDebugScreenSetFgColor(COLOR_WHITE);
		printf(" [%c] Dump full game\n", sel == 0 ? 'X' : ' ');
		printf(" [%c] Dump update\n", sel == 1 ? 'X' : ' ');
		printf(" [%c] Exit\n", sel == 2 ? 'X' : ' ');

		SceCtrlData pad;
		memset(&pad, 0, sizeof(SceCtrlData));
		sceCtrlPeekBufferPositive(0, &pad, 1);

		pressed_buttons = pad.buttons & ~current_buttons;
		current_buttons = pad.buttons;

		if (pressed_buttons & SCE_CTRL_UP) {
			if (sel > 0)
				sel--;
		} else if (pressed_buttons & SCE_CTRL_DOWN) {
			if (sel < 2)
				sel++;
		} else if (pressed_buttons & SCE_CTRL_CROSS) {
			break;
		}

		sceDisplayWaitVblankStart();
	}

	printf("\n\n");

	switch (sel) {
		case 0:
			dumpGame();
			break;
		case 1:
			dumpUpdate();
			break;
	}

	printf("Auto-exiting in 10 seconds...\n");

	sceKernelDelayThread(10 * 1000 * 1000);
	sceKernelExitProcess(0);

	return 0;
}
