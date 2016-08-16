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

#ifndef __FIOS2_H__
#define __FIOS2_H__

/*
	160 0x58
	165 0x58
	210 0x5C
	210 0x5C
	315 0x60
	330 0x60
	336 0x60
	350 0x74
	360 0x74
*/

// threadPriority: 0x42, 0xBD
// threadAffinity: 0x40000, 0x0

typedef struct {
	void *pPtr;
	size_t length;
} SceFiosBuffer;

// Used in PCSF00007 and PCSF00247
typedef struct {
	uint32_t initialized : 1; // 0x00
	uint32_t paramsSize : 15;
	uint32_t pathMax : 16; // 0x02
	uint32_t profiling; // 0x04
	uint32_t ioThreadCount; // 0x08
	uint32_t extraFlags; // 0x0C
	uint32_t maxChunk; // 0x10
	uint8_t maxDecompressorThreadCount; // 0x14
	uint8_t reserved1; // 0x15
	uint8_t reserved2; // 0x16
	uint8_t reserved3; // 0x17
	intptr_t reserved4; // 0x18
	intptr_t reserved5; // 0x1C
	SceFiosBuffer opStorage; // 0x20
	SceFiosBuffer fhStorage; // 0x28
	SceFiosBuffer dhStorage; // 0x30
	SceFiosBuffer chunkStorage; // 0x38
	void *pVprintf; // 0x40
	void *pMemcpy; // 0x44
	int threadPriority[2]; // 0x48
	int threadAffinity[2]; // 0x50
} SceFiosParams58; // 0x58

// Used in PCSB00411 and batman
typedef struct {
	uint32_t initialized : 1; // 0x00
	uint32_t paramsSize : 15;
	uint32_t pathMax : 16; // 0x02
	uint32_t profiling; // 0x04
	uint32_t ioThreadCount; // 0x08
	uint32_t extraValue; // 0x0C ADDED. VALUE: 1
	uint32_t extraFlags; // 0x10
	uint32_t maxChunk; // 0x14
	uint8_t maxDecompressorThreadCount; // 0x18
	uint8_t reserved1; // 0x19
	uint8_t reserved2; // 0x1A
	uint8_t reserved3; // 0x1B
	intptr_t reserved4; // 0x1C
	intptr_t reserved5; // 0x20
	SceFiosBuffer opStorage; // 0x24
	SceFiosBuffer fhStorage; // 0x2C
	SceFiosBuffer dhStorage; // 0x34
	SceFiosBuffer chunkStorage; // 0x3C
	void *pVprintf; // 0x44
	void *pMemcpy; // 0x48
	int threadPriority[2]; // 0x4C
	int threadAffinity[2]; // 0x54
} SceFiosParams5C; // 0x5C

// Used in PCSF00560
typedef struct {
	uint32_t initialized : 1; // 0x00
	uint32_t paramsSize : 15;
	uint32_t pathMax : 16; // 0x02
	uint32_t profiling; // 0x04
	uint32_t ioThreadCount; // 0x08
	uint32_t extraValue; // 0x0C ADDED. VALUE: 1
	uint32_t extraFlags; // 0x10
	uint32_t maxChunk; // 0x14
	uint8_t maxDecompressorThreadCount; // 0x18
	uint8_t reserved1; // 0x19
	uint8_t reserved2; // 0x1A
	uint8_t reserved3; // 0x1B
	intptr_t reserved4; // 0x1C
	intptr_t reserved5; // 0x20
	SceFiosBuffer opStorage; // 0x24
	SceFiosBuffer fhStorage; // 0x2C
	SceFiosBuffer dhStorage; // 0x34
	SceFiosBuffer chunkStorage; // 0x3C
	void *pVprintf; // 0x44
	void *pMemcpy; // 0x48
	void *pExtraCallback; // 0x4C ADDED. VALUE: 0
	int threadPriority[2]; // 0x50
	int threadAffinity[2]; // 0x58
} SceFiosParams60; // 0x60

typedef struct {
	uint32_t initialized : 1; // 0x00
	uint32_t paramsSize : 15;
	uint32_t pathMax : 16; // 0x02
	uint32_t profiling; // 0x04
	uint32_t ioThreadCount; // 0x08
	uint32_t extraValue; // 0x0C ADDED. VALUE: 1
	uint32_t extraFlags; // 0x10
	uint32_t maxChunk; // 0x14
	uint8_t maxDecompressorThreadCount; // 0x18
	uint8_t reserved1; // 0x19
	uint8_t reserved2; // 0x1A
	uint8_t reserved3; // 0x1B
	intptr_t reserved4; // 0x1C
	intptr_t reserved5; // 0x20
	SceFiosBuffer opStorage; // 0x24
	SceFiosBuffer fhStorage; // 0x2C
	SceFiosBuffer dhStorage; // 0x34
	SceFiosBuffer chunkStorage; // 0x3C
	void *pVprintf; // 0x40
	void *pMemcpy; // 0x48 OK
	void *pExtraCallback; // 0x4C ADDED. VALUE: 0
	int threadPriority[3]; // 0x50: 0x42, 0xbd, 0x42
	int threadAffinity[3]; // 0x5C:
	int threadStackSize[3]; // 0x68: 0x2000, 0x4000, 0x2000
} SceFiosParams74; // 0x74

int sceFiosInitialize(void *param);

#endif