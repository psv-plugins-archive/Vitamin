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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <psp2/moduleinfo.h>

#define MAX_PATH_LENGTH 1024
#define TRANSFER_SIZE 64 * 1024
#define SCE_ERROR_ERRNO_EEXIST 0x80010011

typedef struct SfoHeader {
	uint32_t magic;
	uint32_t version;
	uint32_t keyofs;
	uint32_t valofs;
	uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoEntry {
	uint16_t nameofs;
	uint8_t  alignment;
	uint8_t  type;
	uint32_t valsize;
	uint32_t totalsize;
	uint32_t dataofs;
} __attribute__((packed)) SfoEntry;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t num_tls_vars;
	uint32_t reserved1;
	uint32_t module_nid;
	char *lib_name;
	uint32_t reserved2;
	uint32_t *func_nid_table;
	void **func_entry_table;
	uint32_t *var_nid_table;
	void **var_entry_table;
	uint32_t *tls_nid_table;
	void **tls_entry_table;
} SceImportsTable2xx;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t unknown1;
	uint32_t module_nid;
	char *lib_name;
	uint32_t *func_nid_table;
	void **func_entry_table;
	uint32_t *var_nid_table;
	void **var_entry_table;
} SceImportsTable3xx;

int allocateReadFile(char *path, void **buffer);

int ReadFile(char *file, void *buf, int size);
int WriteFile(char *file, void *buf, int size);

int copyFile(char *src_path, char *dst_path);
int copyPath(char *src_path, char *dst_path);
int removePath(char *path);
int getPathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files, int (* handler)(char *path));

void getSizeString(char *string, uint64_t size);

int getSfoString(void *buffer, char *name, char *string, int length);

uint32_t findModuleImport(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid);

#endif
