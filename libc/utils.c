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
#include <psp2/moduleinfo.h>
#include <psp2/power.h>
#include <psp2/sysmodule.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "main.h"
#include "utils.h"
#include "graphics.h"

int debugPrintf(char *text, ...) {
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	char path[128];
	sprintf(path, "%s/libc_log.txt", pspemu_path);
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

int copyFile(char *src_path, char *dst_path) {
	printf("Writing ux0:pspemu/%s...", dst_path + sizeof(pspemu_path));

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

int gzipCompress(uint8_t *dst, uint8_t *src, int size) {
	int res;

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	uint8_t *buffer = malloc(size);
	if (!buffer)
		return -1;

	res = deflateInit2(&strm, 9, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
	if (res != Z_OK) {
		free(buffer);
		return res;
	}

	strm.avail_in = size;
	strm.next_in = (void *)src;
	strm.avail_out = size;
	strm.next_out = buffer;

	res = deflate(&strm, Z_FINISH);
	if (res == Z_STREAM_ERROR) {
		deflateEnd(&strm);
		free(buffer);
		return res;
	}

	memcpy(dst, buffer, strm.total_out);

	deflateEnd(&strm);
	free(buffer);

	return strm.total_out;
}

void convertToImportsTable3xx(SceImportsTable2xx *import_2xx, SceImportsTable3xx *import_3xx) {
	memset(import_3xx, 0, sizeof(SceImportsTable3xx));

	if (import_2xx->size == sizeof(SceImportsTable2xx)) {
		import_3xx->size = import_2xx->size;
		import_3xx->lib_version = import_2xx->lib_version;
		import_3xx->attribute = import_2xx->attribute;
		import_3xx->num_functions = import_2xx->num_functions;
		import_3xx->num_vars = import_2xx->num_vars;
		import_3xx->module_nid = import_2xx->module_nid;
		import_3xx->lib_name = import_2xx->lib_name;
		import_3xx->func_nid_table = import_2xx->func_nid_table;
		import_3xx->func_entry_table = import_2xx->func_entry_table;
		import_3xx->var_nid_table = import_2xx->var_nid_table;
		import_3xx->var_entry_table = import_2xx->var_entry_table;
	} else if (import_2xx->size == sizeof(SceImportsTable3xx)) {
		memcpy(import_3xx, import_2xx, sizeof(SceImportsTable3xx));
	}
}

uint32_t findModuleImport(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid) {
	if (!mod_info)
		return 0;

	uint32_t i = mod_info->impTop;
	while (i < mod_info->impBtm) {
		SceImportsTable3xx import;
		convertToImportsTable3xx((void *)text_addr + i, &import);

		if (import.lib_name && strcmp(import.lib_name, libname) == 0) {
			int j;
			for (j = 0; j < import.num_functions; j++) {
				if (import.func_nid_table[j] == nid) {
					return (uint32_t)import.func_entry_table[j];
				}
			}
		}

		i += import.size;
	}

	return 0;
}