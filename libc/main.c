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
#include <psp2/moduleinfo.h>
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

#include <zlib.h>

#include "main.h"
#include "utils.h"
#include "graphics.h"

#include "self.h"
#include "elf.h"

#include "../reserved_data.h"

int _newlib_heap_size_user = 64 * 1024 * 1024;

extern uint32_t payload_start, payload_end;

#define PLUGIN_PATH "ux0:freek.suprx"

static char contentid[48];
static char titleid[17];

char pspemu_path[16];

typedef struct {
	char *path;
	int add_payload;
	uint32_t text_addr;
	uint32_t text_size;
	uint32_t data_addr;
	uint32_t data_size;
	uint32_t sce_module_info_offset;
} SuprxArguments;

// ux0:app/ABCD01234/sce_module/steriod.suprx

// TODO: compress
// TODO: dump additional suprx

// TODO: pgf font?
// TODO: appmgr api to launch manual

int writeSuprx(SuprxArguments *args) {
	// Backup real text size and data size
	uint32_t real_text_size = args->text_size;
	uint32_t real_data_size = args->data_size;

	// Payload size and plugin path size
	uint32_t payload_size = (&payload_end - &payload_start) * sizeof(uint32_t);

	uint32_t available_text_space = args->data_addr - args->text_addr - args->text_size;
	debugPrintf("Available text space for payload: 0x%08X\n", available_text_space);

	// Check if there is space for the payload
	if (payload_size > available_text_space) {
		args->add_payload = 0;
	}

	// Payload address
	uint32_t payload_addr = args->text_size;

	// Reserve space in text segment for module_start hijacking
	if (args->add_payload) {
		args->text_size = ALIGN(args->text_size, sizeof(uint32_t));
		args->text_size += payload_size;
	}

	// Reserved data address
	uint32_t reserved_data_addr = args->data_size;

	// Reserve space in data segment
	args->data_size += sizeof(ReservedData);

	// Copy .text to buffer
	char *text_buf = malloc(args->text_size);
	memset(text_buf, 0, args->text_size);
	memcpy(text_buf, (void *)args->text_addr, real_text_size);

	// Copy .data to buffer
	char *data_buf = malloc(args->data_size);
	memset(data_buf, 0, args->data_size);
	memcpy(data_buf, (void *)args->data_addr, real_data_size);

	// Reserved data
	ReservedData *reserved_data = (ReservedData *)(data_buf + reserved_data_addr);

	// Copy plugin path to reserved space in data
	strcpy(reserved_data->path, PLUGIN_PATH);

	//hijackStub(args, "SceFios2", 0x774C2C05, 0);
/*
int hijackStub(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid, int stub_position) {
	return 0;
}
*/
	// Hijack sceFiosInitialize
	uint32_t function = findModuleImport((SceModuleInfo *)(args->text_addr + args->sce_module_info_offset), args->text_addr, "SceFios2", 0x774C2C05);
	if (function) {
		uint32_t addr = function - args->text_addr;

		uint32_t address = args->data_addr + reserved_data_addr + (uint32_t)&(((ReservedData *)NULL)->stubs[0]);
		*(uint32_t *)(text_buf + addr + 0x0) = 0xE3004000 | (((address & 0xFFFF) & 0xF000) << 4) | ((address & 0xFFFF) & 0xFFF); // movw v1, #low
		*(uint32_t *)(text_buf + addr + 0x4) = 0xE3404000 | (((address >> 16) & 0xF000) << 4) | ((address >> 16) & 0xFFF); // movt v1, #high
		*(uint32_t *)(text_buf + addr + 0x8) = 0xE5944000; // ldr v1, [v1]
		*(uint32_t *)(text_buf + addr + 0xC) = 0xE12FFF14; // bx v1

		// With this patch, 0x774C2C05 will be resolved in function + 0x10
		// I checked, this does not corrupt import
		int i;
		for (i = args->sce_module_info_offset; i < args->text_size; i += 4) {
			if (*(uint32_t *)(text_buf + i) == function) {
				*(uint32_t *)(text_buf + i) = function + 0x10;
			}
		}
	}

	// TODO: field_38 in sce_module_info. what is it?

	// Add payload
	if (args->add_payload) {
		// 0x4373B548 __sce_aeabi_idiv0
		// 0xFB235848 __sce_aeabi_ldiv0

		uint32_t function = findModuleImport((SceModuleInfo *)(args->text_addr + args->sce_module_info_offset), args->text_addr, "SceLibKernel", 0x4373B548);
		debugPrintf("function: 0x%08X\n", function);
		if (function) {
			// Hijack __sce_aeabi_idiv0 to sceKernelLoadStartModule
			int i;
			for (i = args->sce_module_info_offset; i < args->text_size; i += 4) {
				if (*(uint32_t *)(text_buf + i) == 0x4373B548) {
					*(uint32_t *)(text_buf + i) = 0x2DCC4AFA; // sceKernelLoadStartModule
				}
			}

			// Copy payload
			memcpy(text_buf + payload_addr, &payload_start, payload_size);

			// Get original module_start
			uint32_t module_start_ori = *(uint32_t *)(text_buf + args->sce_module_info_offset + 0x44);

			debugPrintf("module_start_ori: 0x%08X\n", module_start_ori);

			// module_start redirection #1
			*(uint32_t *)(text_buf + args->sce_module_info_offset + 0x44) = payload_addr;

			// module_start redirection #2
			for (i = args->sce_module_info_offset; i < args->text_size; i += 4) {
				if (*(uint32_t *)(text_buf + i) == (args->text_addr + module_start_ori + 1)) {
					*(uint32_t *)(text_buf + i) = (args->text_addr + payload_addr + 1);
					break;
				}
			}

			// Adjust args
			*(uint32_t *)(text_buf + payload_addr + 0x30) = args->data_addr + reserved_data_addr + (uint32_t)&(((ReservedData *)NULL)->path); // path
			*(uint32_t *)(text_buf + payload_addr + 0x34) = function; // sceKernelLoadStartModule
			*(uint32_t *)(text_buf + payload_addr + 0x38) = args->text_addr + module_start_ori + 1; // module_start
		}
	}

	// Offsets
	uint32_t text_offset = ALIGN(sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr) * PROGRAM_HEADER_NUM, 0x10);
	uint32_t data_offset = text_offset + ALIGN(args->text_size, 0x1000);

	// ELF padding
	uint32_t pad[3];
	memset(pad, 0, sizeof(pad));

	// SCE header
	SCE_header shdr;
	memset(&shdr, 0, sizeof(SCE_header));
	shdr.magic = 0x00454353; // "SCE\0"
	shdr.version = 3;
	shdr.sdk_type = 0xC0;
	shdr.header_type = 1;
	shdr.metadata_offset = 0x600;
	shdr.header_len = HEADER_LEN;
	shdr.elf_filesize = data_offset + args->data_size;
	shdr.self_offset = 4;
	shdr.appinfo_offset = sizeof(SCE_header);
	shdr.elf_offset = sizeof(SCE_header) + sizeof(SCE_appinfo);
	shdr.phdr_offset = shdr.elf_offset + sizeof(Elf32_Ehdr) + sizeof(pad);
	shdr.section_info_offset = shdr.phdr_offset + sizeof(Elf32_Phdr) * PROGRAM_HEADER_NUM;
	shdr.sceversion_offset = shdr.section_info_offset + sizeof(segment_info) * PROGRAM_HEADER_NUM;
	shdr.controlinfo_offset = shdr.sceversion_offset + sizeof(SCE_version);
	shdr.controlinfo_size = sizeof(SCE_controlinfo_5) + sizeof(SCE_controlinfo_6) + sizeof(SCE_controlinfo_7);
	shdr.self_filesize = shdr.sceversion_offset + shdr.elf_filesize;

	// App info
	SCE_appinfo appinfo;
	memset(&appinfo, 0, sizeof(SCE_appinfo));
	appinfo.authid = 0x2F00000000000001ULL;
	appinfo.vendor_id = 0;
	appinfo.self_type = 8;
	appinfo.version = 0x1000000000000;
	appinfo.padding = 0;

	// SCE version
	SCE_version ver;
	memset(&ver, 0, sizeof(SCE_version));
	ver.unk1 = 1;
	ver.unk2 = 0;
	ver.unk3 = 16;
	ver.unk4 = 0;

	// Control info 5
	SCE_controlinfo_5 ctrl_5;
	memset(&ctrl_5, 0, sizeof(SCE_controlinfo_5));
	ctrl_5.common.type = 5;
	ctrl_5.common.size = sizeof(ctrl_5);
	ctrl_5.common.unk = 1;

	// Control info 6
	SCE_controlinfo_6 ctrl_6;
	memset(&ctrl_6, 0, sizeof(SCE_controlinfo_6));
	ctrl_6.common.type = 6;
	ctrl_6.common.size = sizeof(ctrl_6);
	ctrl_6.common.unk = 1;
	ctrl_6.unk1 = 1;

	// Control info 7
	SCE_controlinfo_7 ctrl_7;
	memset(&ctrl_7, 0, sizeof(SCE_controlinfo_7));
	ctrl_7.common.type = 7;
	ctrl_7.common.size = sizeof(ctrl_7);

	// ELF header
	Elf32_Ehdr ehdr;
	memset(&ehdr, 0, sizeof(Elf32_Ehdr));
	memcpy(ehdr.e_ident, "\177ELF\1\1\1", 8);
	ehdr.e_type = ET_SCE_EXEC;
	ehdr.e_machine = EM_ARM;
	ehdr.e_version = EV_CURRENT;
	ehdr.e_entry = args->sce_module_info_offset;
	ehdr.e_phoff = sizeof(Elf32_Ehdr);
	ehdr.e_flags = 0x05000000;
	ehdr.e_ehsize = sizeof(Elf32_Ehdr);
	ehdr.e_phentsize = sizeof(Elf32_Phdr);
	ehdr.e_phnum = PROGRAM_HEADER_NUM;

	// Make .text program header
	Elf32_Phdr phdr_text;
	memset(&phdr_text, 0, sizeof(Elf32_Phdr));
	phdr_text.p_type = PT_LOAD;
	phdr_text.p_offset = text_offset;
	phdr_text.p_vaddr = args->text_addr;
	phdr_text.p_paddr = 0;
	phdr_text.p_filesz = args->text_size;
	phdr_text.p_memsz = args->text_size;
	phdr_text.p_flags = PF_R | PF_X;
	phdr_text.p_align = 0x10;

	// Make .data program header
	Elf32_Phdr phdr_data;
	memset(&phdr_data, 0, sizeof(Elf32_Phdr));
	phdr_data.p_type = PT_LOAD;
	phdr_data.p_offset = data_offset;
	phdr_data.p_vaddr = args->data_addr;
	phdr_data.p_paddr = 0;
	phdr_data.p_filesz = args->data_size;
	phdr_data.p_memsz = args->data_size;
	phdr_data.p_flags = PF_R | PF_W;
	phdr_data.p_align = 0x10;

	// Make .text segment info
	segment_info sinfo_text;
	memset(&sinfo_text, 0, sizeof(segment_info));
	sinfo_text.offset = HEADER_LEN + text_offset;
	sinfo_text.length = args->text_size;
	sinfo_text.compression = 1;
	sinfo_text.encryption = 2;

	// Make .data segment info
	segment_info sinfo_data;
	memset(&sinfo_data, 0, sizeof(segment_info));
	sinfo_data.offset = HEADER_LEN + data_offset;
	sinfo_data.length = args->data_size;
	sinfo_data.compression = 1;
	sinfo_data.encryption = 2;

	// Write suprx to ux0:pspemu/TITLEID/
	char path[128];
	sprintf(path, "%s/%s/%s", pspemu_path, titleid, args->path + strlen("app0:"));
	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	// Write SELF
	sceIoWrite(fd, &shdr, sizeof(SCE_header));
	sceIoWrite(fd, &appinfo, sizeof(SCE_appinfo));
	sceIoWrite(fd, &ehdr, sizeof(Elf32_Ehdr));
	sceIoWrite(fd, &pad, sizeof(pad));
	sceIoWrite(fd, &phdr_text, sizeof(Elf32_Phdr));
	sceIoWrite(fd, &phdr_data, sizeof(Elf32_Phdr));
	sceIoWrite(fd, &sinfo_text, sizeof(segment_info));
	sceIoWrite(fd, &sinfo_data, sizeof(segment_info));
	sceIoWrite(fd, &ver, sizeof(SCE_version));
	sceIoWrite(fd, &ctrl_5, sizeof(SCE_controlinfo_5));
	sceIoWrite(fd, &ctrl_6, sizeof(SCE_controlinfo_6));
	sceIoWrite(fd, &ctrl_7, sizeof(SCE_controlinfo_7));

	// Write ELF
	sceIoLseek(fd, HEADER_LEN, SCE_SEEK_SET);
	sceIoWrite(fd, &ehdr, sizeof(Elf32_Ehdr));
	sceIoWrite(fd, &phdr_text, sizeof(Elf32_Phdr));
	sceIoWrite(fd, &phdr_data, sizeof(Elf32_Phdr));

	sceIoLseek(fd, HEADER_LEN + text_offset, SCE_SEEK_SET);
	sceIoWrite(fd, (void *)text_buf, args->text_size);

	sceIoLseek(fd, HEADER_LEN + data_offset, SCE_SEEK_SET);
	sceIoWrite(fd, (void *)data_buf, args->data_size);

	free(text_buf);

	// Close
	sceIoClose(fd);

	return 0;
}

int dumpSuprx(char *path, int flags, int add_payload) {
	// Load module, but not start. NID unpoisoned
	SceUID mod = sceKernelLoadModule(path, flags, NULL);
	if (mod < 0)
		return mod;

	// Get module info
	SceKernelModuleInfo info;
	info.size = sizeof(SceKernelModuleInfo);
	int res = sceKernelGetModuleInfo(mod, &info);
	if (res < 0)
		return res;

	// Open suprx
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	// Read SCE header
	SCE_header shdr;
	sceIoRead(fd, &shdr, sizeof(SCE_header));

	// Read ELF header
	Elf32_Ehdr ehdr;
	sceIoLseek(fd, shdr.elf_offset, SCE_SEEK_SET);
	sceIoRead(fd, &ehdr, sizeof(Elf32_Ehdr));

	// Close
	sceIoClose(fd);

	// Write suprx
	SuprxArguments args;
	args.path = path;
	args.add_payload = add_payload;
	args.text_addr = (uint32_t)info.segments[0].vaddr;
	args.text_size = (uint32_t)info.segments[0].memsz;
	args.data_addr = (uint32_t)info.segments[1].vaddr;
	args.data_size = (uint32_t)info.segments[1].memsz;
	args.sce_module_info_offset = ehdr.e_entry;

	debugPrintf("titleid: %s\n", titleid);
	debugPrintf("path: %s\n", args.path);
	debugPrintf("text_addr: 0x%08X\n", args.text_addr);
	debugPrintf("text_size: 0x%08X\n", args.text_size);
	debugPrintf("data_addr: 0x%08X\n", args.data_addr);
	debugPrintf("data_size: 0x%08X\n", args.data_size);
	debugPrintf("sce_module_info_offset: 0x%08X\n", args.sce_module_info_offset);

	// TODO: remove this. DEBUG ONLY: dump text and addr separately
	char string[128];

	sprintf(string, "%s/%s_0_0x%08X", pspemu_path, info.module_name, (unsigned int)args.text_addr);
	WriteFile(string, (void *)args.text_addr, args.text_size);

	sprintf(string, "%s/%s_1_0x%08X", pspemu_path, info.module_name, (unsigned int)args.data_addr);
	WriteFile(string, (void *)args.data_addr, args.data_size);

	writeSuprx(&args);

	return 0;
}

int dumpEbootBin() {
	int res;

	SceUID mod_list[MAX_MODULES];
	int mod_count = MAX_MODULES;

	// Get module list
	res = sceKernelGetModuleList(0xFF, mod_list, &mod_count);
	if (res < 0)
		return res;

	// Stop & unload
	res = sceKernelStopUnloadModule(mod_list[mod_count - 1], 0, NULL, 0, NULL, NULL);
	if (res < 0)
		return res;

	// Dump eboot.bin
	char path[128];
	sprintf(path, "%s/%s", pspemu_path, titleid);
	sceIoMkdir(path, 0777);

	dumpSuprx("app0:eboot.bin", 0, 1);

	return 0;
}

int dumpGame() {
	char path[128];
	sprintf(path, "%s/%s", pspemu_path, titleid);
	copyPath("app0:", path);
	return 0;
}

void printInfo() {
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("\nVitamin\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("by Team FreeK\n\n");
}

int main(int argc, char *argv[]) {
	SceUID fd;
	char path[128];

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

	sprintf(path, "%s/updt.bin", pspemu_path);
	if ((fd = sceIoOpen(path, SCE_O_RDONLY, 0)) > 0) {
		sceIoClose(fd);
		sceIoRemove(path);
		strcat(titleid, "_updt");
	}

	printf("Writing ux0:pspemu/%s/eboot.bin...", titleid);
	dumpEbootBin();
	printf("OK\n");

	printf("Auto-exiting in 5 seconds...\n");

	sceKernelDelayThread(5 * 1000 * 1000);
	sceKernelExitProcess(0);

	return 0;
}
