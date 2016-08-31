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

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"

#include "../common/utils.h"
#include "../common/common.h"

int addGames(char *app_path, int is_cartridge, int count, GameInfo *game_infos, char **game_entries) {
	SceUID dfd = sceIoDopen(app_path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char path[128];
				sprintf(path, "%s/%s/sce_sys/clearsign", app_path, dir.d_name);

				SceIoStat stat;
				memset(&stat, 0, sizeof(SceIoStat));
				if (sceIoGetstat(path, &stat) >= 0) {
					void *buffer = NULL;

					// Read app param.sfo
					sprintf(path, "%s/%s/sce_sys/param.sfo", app_path, dir.d_name);
					if (allocateReadFile(path, &buffer) <= 0)
						continue;

					// Clear
					memset(&game_infos[count], 0, sizeof(GameInfo));

					// Is cartridge
					game_infos[count].is_cartridge = is_cartridge;

					// Has grw0?
					if (is_cartridge) {
						SceIoStat stat;
						memset(&stat, 0, sizeof(SceIoStat));
						if (sceIoGetstat("grw0:", &stat) >= 0)
							game_infos[count].has_grw0 = 1;
					}

					// Get title
					getSfoString(buffer, "TITLE", game_infos[count].name, 40 - 1);

					// Get title id
					getSfoString(buffer, "TITLE_ID", game_infos[count].titleid, 12);

					// Get version
					getSfoString(buffer, "APP_VER", game_infos[count].version_game, 8);

					// Copy game version
					strcpy(game_infos[count].version, game_infos[count].version_game);

					// Free buffer
					free(buffer);
					buffer = NULL;

					// Read patch param.sfo
					if (!is_cartridge) {
						sprintf(path, "ux0:patch/%s/sce_sys/param.sfo", dir.d_name);

						if (allocateReadFile(path, &buffer) > 0) {
							// Get version
							getSfoString(buffer, "APP_VER", game_infos[count].version_update, 8);

							// Copy patch version
							strcpy(game_infos[count].version, game_infos[count].version_update);

							// Free buffer
							free(buffer);
							buffer = NULL;
						}
					}

					// Name
					char *name = malloc(256);
					sprintf(name, "[%s] %s", game_infos[count].titleid, game_infos[count].name);
					game_entries[count] = name;

					count++;
				}
			}
		} while (res > 0 && count < MAX_GAMES);

		sceIoDclose(dfd);
	}

	return count;
}

int getGames(GameInfo *game_infos, char **game_entries) {
	int count = 0;

	// Open cartridge app folder
	count = addGames("gro0:app", 1, count, game_infos, game_entries);

	// Open app folder
	count = addGames("ux0:app", 0, count, game_infos, game_entries);
/*
	int i;
	for (i = 0; i < 20; i++) {
		char *name = malloc(256);
		sprintf(name, "Test %d", i);
		game_entries[count] = name;
		count++;
	}
*/
	return count;
}