/*
 *   utils.h
 *   Copyright (C) 2021 David García Goñi <dagargo@gmail.com>
 *
 *   This file is part of Overwitch.
 *
 *   Overwitch is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Overwitch is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Overwitch. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILS_H
#define UTILS_H

#define emu3_print(level, indent, ...) {\
		if (level <= verbosity) { \
			for (int i = 0; i < indent; i++) \
				printf("  "); \
			printf(__VA_ARGS__); \
		}\
	}

#define emu3_debug(level, format, ...) if (level <= verbosity) fprintf(stderr, "DEBUG:" __FILE__ ":%d:(%s): " format, __LINE__, __FUNCTION__, ## __VA_ARGS__)

#define emu3_error(format, ...) fprintf(stderr, "\x1b[31mERROR:" __FILE__ ":%d:(%s): " format "\x1b[m", __LINE__, __FUNCTION__, ## __VA_ARGS__)

extern int verbosity;

#endif
