/*
 *   emu3bm.h
 *   Copyright (C) 2018 David García Goñi <dagargo@gmail.com>
 *
 *   This file is part of emu3bm.
 *
 *   emu3bm is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   emu3bm is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with emu3bm.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sndfile.h>
#include <stdlib.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <libgen.h>
#include "../config.h"

#define DEVICE_ESI2000 "esi2000"
#define DEVICE_EMU3X "emu3x"

#define NOTES 88		// 0x58

extern int verbosity;

extern const char *note_names[];

#define emu3_log(level, indent, ...) {\
		if (level <= verbosity) { \
			for (int i = 0; i < indent; i++) \
				printf("  "); \
			printf(__VA_ARGS__); \
		}\
	}

struct emu3_file
{
  const char *filename;
  union
  {
    char *raw;
    struct emu3_bank *bank;
  };
  size_t fsize;
};

struct emu3_zone_range
{
  int layer;
  int original_key;
  int lower_key;
  int higher_key;
};

struct emu3_file *emu3_open_file (const char *);

void emu3_close_file (struct emu3_file *);

void emu3_write_file (struct emu3_file *);

int emu3_add_sample (struct emu3_file *, char *, int);

int emu3_add_preset (struct emu3_file *, char *);

int emu3_add_preset_zone (struct emu3_file *, int preset_num, int sample_num,
			  struct emu3_zone_range *);

int emu3_extract_samples (struct emu3_file *);

int emu3_process_bank (struct emu3_file *, int, int, char *, int, int, int,
		       int, int);

int emu3_create_bank (const char *, const char *);

int emu3_reverse_note_search (char *);
