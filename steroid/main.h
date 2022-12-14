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

#ifndef __MAIN_H__
#define __MAIN_H__

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define SCE_APPUTIL_APPPARAM_ID_SKU_FLAG 0

#define SCE_APPUTIL_APPPARAM_SKU_FLAG_NONE 0
#define SCE_APPUTIL_APPPARAM_SKU_FLAG_TRIAL 1
#define SCE_APPUTIL_APPPARAM_SKU_FLAG_FULL 3

#define SCE_APPMGR_APP_PARAM_TITLE_ID 12

#define MAX_MODULES 128

int sceKernelGetProcessId();

#endif