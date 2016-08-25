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

#ifndef __MAKEZIP_H__
#define __MAKEZIP_H__

#define WRITEBUFFERSIZE (16 * 1024)
#define COMPRESS_LEVEL 1

int makeZip(char *zip_file, char *path, int filename_start, int append, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* handler)(char *path));

#endif
