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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "makezip.h"
#include "zip.h"

#include "../common/utils.h"
#include "../common/graphics.h"

uLong filetime(const char *filename, tm_zip *tmzip, uLong *dostime) {
	struct stat s;
	memset(&s, 0, sizeof(struct stat));
	if (stat(filename, &s) != 0)
		return 0;

	struct tm *filedate = localtime(&s.st_mtime);

	tmzip->tm_sec  = filedate->tm_sec;
	tmzip->tm_min  = filedate->tm_min;
	tmzip->tm_hour = filedate->tm_hour;
	tmzip->tm_mday = filedate->tm_mday;
	tmzip->tm_mon  = filedate->tm_mon ;
	tmzip->tm_year = filedate->tm_year;

	return 1;
}

int zipAddFile(zipFile zf, char *path, int filename_start, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* handler)(char *path)) {
	int res;

	// Handler
	if (handler && handler(path)) {
		return 0;
	}

	// Get information about the file on disk so we can store it in zip
	zip_fileinfo zi;
	memset(&zi, 0, sizeof(zip_fileinfo));
	filetime(path, &zi.tmz_date, &zi.dosDate);

	// Size
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	res = sceIoGetstat(path, &stat);
	if (res < 0)
		return res;

	// Large file?
	int use_zip64 = (stat.st_size >= 0xFFFFFFFF);

	// Open new file in zip
	res = zipOpenNewFileInZip3_64(zf, path + filename_start, &zi,
				NULL, 0, NULL, 0, NULL,
				(COMPRESS_LEVEL != 0) ? Z_DEFLATED : 0,
				COMPRESS_LEVEL, 0,
				-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
				NULL, 0, use_zip64);

	if (res < 0)
		return res;

	char size_string[16];
	getSizeString(size_string, stat.st_size);
	char *p = strrchr(path, '/');
	psvDebugScreenPrintf("Writing %s (%s)...", p + 1, size_string);
	int x = psvDebugScreenGetX();
	int y = psvDebugScreenGetY();
	psvDebugScreenClearLineMargin(y + 1, DARKBLUE);

	// Open file to add
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0) {
		psvDebugScreenSetXY(x, y);
		psvDebugScreenPrintf("Error 0x%08X\n", fd);
		zipCloseFileInZip(zf);
		return fd;
	}

	// Add file to zip
	void *buf = malloc(WRITEBUFFERSIZE);

	uint64_t total = 0;

	int read;
	while ((read = sceIoRead(fd, buf, WRITEBUFFERSIZE)) > 0) {
		int res = zipWriteInFileInZip(zf, buf, read);
		if (res < 0) {
			psvDebugScreenSetXY(x, y);
			psvDebugScreenPrintf("Error 0x%08X\n", res);

			free(buf);

			sceIoClose(fd);
			zipCloseFileInZip(zf);

			return res;
		}

		if (value)
			(*value) += read;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		total += read;

		if (stat.st_size >= (1 * 1024 * 1024)) { // Show percent for files >= 1MB
			psvDebugScreenSetXY(x, y);
			double progress = (double)((100.0f * (double)total) / (double)stat.st_size);
			psvDebugScreenPrintf("%d%%", (int)progress);
		}
	}

	psvDebugScreenSetXY(x, y);
	psvDebugScreenPrintf("OK  \n");

	free(buf);

	sceIoClose(fd);
	zipCloseFileInZip(zf);

	return 0;
}

int zipAddPath(zipFile zf, char *path, int filename_start, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* handler)(char *path)) {
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

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					ret = zipAddPath(zf, new_path, filename_start, value, max, SetProgress, handler);
				} else {
					ret = zipAddFile(zf, new_path, filename_start, value, max, SetProgress,handler);
				}

				free(new_path);

				if (ret < 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else {
		return zipAddFile(zf, path, filename_start, value, max, SetProgress,handler);
	}

	return 0;
}

int makeZip(char *zip_file, char *path, int filename_start, int append, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* handler)(char *path)) {
	zipFile zf = zipOpen64(zip_file, append ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE);
	if (zf == NULL)
		return -1;

	int res = zipAddPath(zf, path, filename_start, value, max, SetProgress,handler);

	zipClose(zf, NULL);

	return res;
}
