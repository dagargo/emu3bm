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

#include <stdint.h>
#include <unistd.h>

#define MEM_SIZE 0x08000000	//128 MiB
#define NAME_SIZE 16
#define SAMPLE_EXT ".wav"
#define NOTES 88		// 0x58

enum emu_error
{
  ERR_BANK_FULL = 1,
  ERR_BAD_SAMPLE_CHANS,
  ERR_CANT_OPEN_SAMPLE,
  ERR_SAMPLE_LIMIT,
  ERR_PRESET_LIMIT,
  ERR_OPEN_BANK,
  ERR_WRITE_BANK
};

struct emu_file
{
  const char *name;
  char *raw;
  size_t size;
};

#define emu_print(level, indent, ...) { \
		if (level <= verbosity) { \
			for (int i = 0; i < indent; i++) \
				fprintf(stdout, "  "); \
			fprintf(stdout, __VA_ARGS__); \
		}\
	}

#define emu_debug(level, format, ...) { \
                if (level <= verbosity) { \
                        fprintf(stderr, "DEBUG:" __FILE__ ":%d:(%s): " format "\n", __LINE__, __FUNCTION__, ## __VA_ARGS__); \
                } \
        }

#define emu_error(format, ...) { \
                int tty = isatty(fileno(stderr)); \
                const char * color_start = tty ? "\x1b[31m" : ""; \
                const char * color_end = tty ? "\x1b[m" : ""; \
                fprintf(stderr, "%sERROR:" __FILE__ ":%d:(%s): " format "%s\n", color_start, __LINE__, __FUNCTION__, ## __VA_ARGS__, color_end); \
        }

extern int verbosity;

const char *emu_get_err (int);

struct emu_file *emu_open_file (const char *);

void emu_close_file (struct emu_file *);

int emu_write_file (struct emu_file *);

int emu_reverse_note_search (char *);

const char *emu_get_note_name (uint8_t);

#endif
