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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <psp2/power.h>

#define VITAMIN_VERSION_MAJOR 1

#define printf psvDebugScreenPrintf

typedef struct {
	char name[40];
	char titleid[12];
	char version[8];
	char version_game[8];
	char version_update[8];
	uint64_t size;
	int is_cartridge;
} GameInfo;

enum Modes {
	MODE_FULL_GAME,
	MODE_UPDATE,
};

int sceAppMgrDestroyOtherApp();

void printLayout(char *info, char *title);

void initPowerTickThread();

#endif