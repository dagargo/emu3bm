/*
 *	emu3bm.h
 *	Copyright (C) 2016 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   EMU3 Filesystem Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   EMU3 Filesystem Tool is distributed in the hope that it will be useful,
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

#include "../config.h"

#define MEM_SIZE 0x08000000
#define SIGNATURE_SIZE 16
#define NAME_SIZE 16
#define BANK_PARAMETERS 11
#define MORE_BANK_PARAMETERS 4
#define SAMPLE_ADDR_START_EMU_3X 0x1bd2
#define PRESET_START_EMU_3X 0x2b72
#define SAMPLE_OFFSET 0x400000	//This is also the max sample length in bytes
#define MAX_SAMPLES 1000
#define PRESET_SIZE_ADDR_START_EMU_3X 0x17ca
#define MAX_PRESETS 0x100
#define SAMPLE_PARAMETERS 20
#define DEFAULT_SAMPLING_FREQ 44100
#define MONO_SAMPLE 0x00380001
#define STEREO_SAMPLE 0x00780001
#define MONO_SAMPLE_EMULATOR_3X_1 0x0030fe02
#define MONO_SAMPLE_EMULATOR_3X_2 0x0039fe02
#define MONO_SAMPLE_EMULATOR_3X_3 0xff30fe02
#define MONO_SAMPLE_EMULATOR_3X_4 0xff39fe02
#define MONO_SAMPLE_EMULATOR_3X_5 0x0030ffc3

#define ESI_32_V3_DEF     "EMU SI-32 v3   \0"
#define EMULATOR_3X_DEF   "EMULATOR 3X    \0"

#define SAMPLE_EXT ".wav"

#define EMPTY_BANK "res/empty_bank"

struct emu3_bank
{
  char signature[SIGNATURE_SIZE];
  char bank_name[NAME_SIZE];
  unsigned int parameters[BANK_PARAMETERS];
  char bank_name_copy[NAME_SIZE];
  unsigned int more_parameters[MORE_BANK_PARAMETERS];
};

struct emu3_sample
{
  char name[NAME_SIZE];
  unsigned int parameters[SAMPLE_PARAMETERS];
  short int frames[];
};

// This returns a valid fs name (no strange chars in filename) from the emu3fs object name
char *emu3_fs_sample_name (const char *);

//TODO: remove 3rd parameter if not useful
int emu3_append_sample (const char *, struct emu3_sample *, unsigned int,
			int);

int emu3_process_bank (const char *, int, const char *, int);

void emu3_print_sample_info (struct emu3_sample *);

int emu3_get_sample_channels (struct emu3_sample *);

void emu3_write_sample_file (const char *, short *, sf_count_t, int, int);

int emu3_create_bank (const char *);
