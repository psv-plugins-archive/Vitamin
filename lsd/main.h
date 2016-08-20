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

#ifndef __MAIN_H__
#define __MAIN_H__

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define SCE_APPMGR_APP_PARAM_CONTENT_ID 6
#define SCE_APPMGR_APP_PARAM_CATEGORY 8
#define SCE_APPMGR_APP_PARAM_TITLE_ID 12

#define printf psvDebugScreenPrintf

#define MAX_MODULES 128

#define PROGRAM_HEADER_NUM 2

int sceKernelGetProcessId();

#endif