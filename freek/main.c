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

#include "main.h"
#include "utils.h"
#include "fios2.h"

#include "../reserved_data.h"

int sceFiosInitializePatched(void *param) {
	uint16_t size = (*(uint16_t *)param & 0xFFFE) >> 1;
	debugPrintf("sceFiosInitialize original size: 0x%08X\n", size);

	SceFiosParams74 param74;

	if (size == sizeof(SceFiosParams58)) {
		SceFiosParams58 *param58 = (SceFiosParams58 *)param;
		param74.initialized = param58->initialized;
		param74.paramsSize = sizeof(SceFiosParams74);
		param74.pathMax = param58->pathMax;
		param74.profiling = param58->profiling;
		param74.ioThreadCount = param58->ioThreadCount;
		param74.extraValue = 1;
		param74.extraFlags = param58->extraFlags;
		param74.maxChunk = param58->maxChunk;
		param74.maxDecompressorThreadCount = param58->maxDecompressorThreadCount;
		param74.reserved1 = param58->reserved1;
		param74.reserved2 = param58->reserved2;
		param74.reserved3 = param58->reserved3;
		param74.reserved4 = param58->reserved4;
		param74.reserved5 = param58->reserved5;
		param74.opStorage = param58->opStorage;
		param74.fhStorage = param58->fhStorage;
		param74.dhStorage = param58->dhStorage;
		param74.chunkStorage = param58->chunkStorage;
		param74.pVprintf = param58->pVprintf;
		param74.pMemcpy = param58->pMemcpy;
		param74.pExtraCallback = NULL;
		param74.threadPriority[0] = param58->threadPriority[0];
		param74.threadPriority[1] = param58->threadPriority[1];
		param74.threadPriority[2] = 0x42;
		param74.threadAffinity[0] = param58->threadAffinity[0];
		param74.threadAffinity[1] = param58->threadAffinity[1];
		param74.threadAffinity[2] = 0;
		param74.threadStackSize[0] = 0;
		param74.threadStackSize[1] = 0;
		param74.threadStackSize[2] = 0;
	} else if (size == sizeof(SceFiosParams5C)) {
		SceFiosParams5C *param5c = (SceFiosParams5C *)param;
		param74.initialized = param5c->initialized;
		param74.paramsSize = sizeof(SceFiosParams74);
		param74.pathMax = param5c->pathMax;
		param74.profiling = param5c->profiling;
		param74.ioThreadCount = param5c->ioThreadCount;
		param74.extraValue = param5c->extraValue;
		param74.extraFlags = param5c->extraFlags;
		param74.maxChunk = param5c->maxChunk;
		param74.maxDecompressorThreadCount = param5c->maxDecompressorThreadCount;
		param74.reserved1 = param5c->reserved1;
		param74.reserved2 = param5c->reserved2;
		param74.reserved3 = param5c->reserved3;
		param74.reserved4 = param5c->reserved4;
		param74.reserved5 = param5c->reserved5;
		param74.opStorage = param5c->opStorage;
		param74.fhStorage = param5c->fhStorage;
		param74.dhStorage = param5c->dhStorage;
		param74.chunkStorage = param5c->chunkStorage;
		param74.pVprintf = param5c->pVprintf;
		param74.pMemcpy = param5c->pMemcpy;
		param74.pExtraCallback = NULL;
		param74.threadPriority[0] = param5c->threadPriority[0];
		param74.threadPriority[1] = param5c->threadPriority[1];
		param74.threadPriority[2] = 0x42;
		param74.threadAffinity[0] = param5c->threadAffinity[0];
		param74.threadAffinity[1] = param5c->threadAffinity[1];
		param74.threadAffinity[2] = 0;
		param74.threadStackSize[0] = 0;
		param74.threadStackSize[1] = 0;
		param74.threadStackSize[2] = 0;
	} else if (size == sizeof(SceFiosParams60)) {
		SceFiosParams60 *param60 = (SceFiosParams60 *)param;
		param74.initialized = param60->initialized;
		param74.paramsSize = sizeof(SceFiosParams74);
		param74.pathMax = param60->pathMax;
		param74.profiling = param60->profiling;
		param74.ioThreadCount = param60->ioThreadCount;
		param74.extraValue = param60->extraValue;
		param74.extraFlags = param60->extraFlags;
		param74.maxChunk = param60->maxChunk;
		param74.maxDecompressorThreadCount = param60->maxDecompressorThreadCount;
		param74.reserved1 = param60->reserved1;
		param74.reserved2 = param60->reserved2;
		param74.reserved3 = param60->reserved3;
		param74.reserved4 = param60->reserved4;
		param74.reserved5 = param60->reserved5;
		param74.opStorage = param60->opStorage;
		param74.fhStorage = param60->fhStorage;
		param74.dhStorage = param60->dhStorage;
		param74.chunkStorage = param60->chunkStorage;
		param74.pVprintf = param60->pVprintf;
		param74.pMemcpy = param60->pMemcpy;
		param74.pExtraCallback = param60->pExtraCallback;
		param74.threadPriority[0] = param60->threadPriority[0];
		param74.threadPriority[1] = param60->threadPriority[1];
		param74.threadPriority[2] = 0x42;
		param74.threadAffinity[0] = param60->threadAffinity[0];
		param74.threadAffinity[1] = param60->threadAffinity[1];
		param74.threadAffinity[2] = 0;
		param74.threadStackSize[0] = 0;
		param74.threadStackSize[1] = 0;
		param74.threadStackSize[2] = 0;
	}

	/*
		TODO: research threadPriority, threadAffinity, threadStackSize
	*/

	/*
		Tests
	*/

	//OK
	//param74.pExtraCallback = 0x12345678; // crashes

	//OK SCE_KERNEL_ERROR_NO_FREE_PHYSICAL_PAGE
	//param74.threadStackSize[0] = 0x12345678;
	//param74.threadStackSize[1] = 0x98765432;
	//param74.threadStackSize[2] = 1;

	//param74.threadAffinity[0] = 3; // OK SCE_KERNEL_ERROR_ILLEGAL_CPU_AFFINITY_MASK
	//param74.threadAffinity[1] = 3; // crash but no result
	//param74.threadAffinity[2] = 3; // OK SCE_KERNEL_ERROR_ILLEGAL_CPU_AFFINITY_MASK

	//param74.threadPriority[0] = 1; // OK SCE_KERNEL_ERROR_ILLEGAL_PRIORITY
	//param74.threadPriority[1] = 1; // crash but no result
	//param74.threadPriority[2] = 1; // OK SCE_KERNEL_ERROR_ILLEGAL_PRIORITY

	// Default.
	param74.threadPriority[0] = 0;
	param74.threadPriority[1] = 0;
	param74.threadPriority[2] = 0;

	param74.threadAffinity[0] = 0;
	param74.threadAffinity[1] = 0;
	param74.threadAffinity[2] = 0;

	// 0x58 to 0x74 working
	// 0x5C to 0x74 results 0 but crashes in batman
		
	debugPrintf("param.initialized: 0x%08X\n", param74.initialized);
	debugPrintf("param.paramsSize: 0x%08X\n", param74.paramsSize);
	debugPrintf("param.pathMax: 0x%08X\n", param74.pathMax);
	debugPrintf("param.profiling: 0x%08X\n", param74.profiling);
	debugPrintf("param.ioThreadCount: 0x%08X\n", param74.ioThreadCount);
	debugPrintf("param.extraValue: 0x%08X\n", param74.extraValue);
	debugPrintf("param.extraFlags: 0x%08X\n", param74.extraFlags);
	debugPrintf("param.maxChunk: 0x%08X\n", param74.maxChunk);
	debugPrintf("param.maxDecompressorThreadCount: 0x%08X\n", param74.maxDecompressorThreadCount);
	debugPrintf("param.reserved1: 0x%08X\n", param74.reserved1);
	debugPrintf("param.reserved2: 0x%08X\n", param74.reserved2);
	debugPrintf("param.reserved3: 0x%08X\n", param74.reserved3);
	debugPrintf("param.reserved4: 0x%08X\n", param74.reserved4);
	debugPrintf("param.reserved5: 0x%08X\n", param74.reserved5);
	debugPrintf("param.opStorage.pPtr: 0x%08X\n", param74.opStorage.pPtr);
	debugPrintf("param.opStorage.length: 0x%08X\n", param74.opStorage.length);
	debugPrintf("param.fhStorage.pPtr: 0x%08X\n", param74.fhStorage.pPtr);
	debugPrintf("param.fhStorage.length: 0x%08X\n", param74.fhStorage.length);
	debugPrintf("param.dhStorage.pPtr: 0x%08X\n", param74.dhStorage.pPtr);
	debugPrintf("param.dhStorage.length: 0x%08X\n", param74.dhStorage.length);
	debugPrintf("param.chunkStorage.pPtr: 0x%08X\n", param74.chunkStorage.pPtr);
	debugPrintf("param.chunkStorage.length: 0x%08X\n", param74.chunkStorage.length);
	debugPrintf("param.pVprintf: 0x%08X\n", param74.pVprintf);
	debugPrintf("param.pMemcpy: 0x%08X\n", param74.pMemcpy);
	debugPrintf("param.pExtraCallback: 0x%08X\n", param74.pExtraCallback);
	debugPrintf("param.threadPriority[0]: 0x%08X\n", param74.threadPriority[0]);
	debugPrintf("param.threadPriority[1]: 0x%08X\n", param74.threadPriority[1]);
	debugPrintf("param.threadPriority[2]: 0x%08X\n", param74.threadPriority[2]);
	debugPrintf("param.threadAffinity[0]: 0x%08X\n", param74.threadAffinity[0]);
	debugPrintf("param.threadAffinity[1]: 0x%08X\n", param74.threadAffinity[1]);
	debugPrintf("param.threadAffinity[2]: 0x%08X\n", param74.threadAffinity[2]);
	debugPrintf("param.threadStackSize[0]: 0x%08X\n", param74.threadStackSize[0]);
	debugPrintf("param.threadStackSize[1]: 0x%08X\n", param74.threadStackSize[1]);
	debugPrintf("param.threadStackSize[2]: 0x%08X\n", param74.threadStackSize[2]);

	int res = sceFiosInitialize(&param74);

	debugPrintf("%s: 0x%08X\n", __FUNCTION__, res);

	return res;
}

// BATMAN: crash in libc + 0x14198 :(

int _start(SceSize args, void *argp) {
	// Get titleid
	char titleid[12];
	memset(titleid, 0, sizeof(titleid));
	sceAppMgrAppParamGetString(sceKernelGetProcessId(), SCE_APPMGR_APP_PARAM_TITLE_ID, titleid, sizeof(titleid));

	debugPrintf("Freek module loaded with %s\n", titleid);

	// Set CPU clock to 444mhz
	scePowerSetArmClockFrequency(444);

	int res;

	SceUID mod_list[MAX_MODULES];
	int mod_count = MAX_MODULES;

	// Get module list
	res = sceKernelGetModuleList(0xFF, mod_list, &mod_count);
	if (res < 0)
		return res;

	// Get module info
	SceKernelModuleInfo info;
	info.size = sizeof(SceKernelModuleInfo);
	res = sceKernelGetModuleInfo(mod_list[mod_count - 1], &info);
	if (res < 0)
		return res;

	// Adjust stub patch addresses
	uint32_t data_addr = (uint32_t)info.segments[1].vaddr;
	uint32_t data_size = (uint32_t)info.segments[1].memsz;

	// Reserved data
	ReservedData *reserved_data = (ReservedData *)(data_addr + data_size - sizeof(ReservedData));

	// Function #0
	reserved_data->stubs[0] = (uint32_t)sceFiosInitializePatched + 1;

	return 0;
}