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
#include <stdlib.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

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
#define SAMPLE_PARAMETERS 9
#define MORE_SAMPLE_PARAMETERS 8
#define DEFAULT_SAMPLING_FREQ 44100
#define MONO_SAMPLE_1 0x00380001
#define MONO_SAMPLE_2 0x00390001	//Looped?
#define STEREO_SAMPLE_1 0x00780001
#define STEREO_SAMPLE_2 0x00700001	//Looped?
#define MONO_SAMPLE_EMULATOR_3X_1 0x0030fe02
#define MONO_SAMPLE_EMULATOR_3X_2 0x0039fe02
#define MONO_SAMPLE_EMULATOR_3X_3 0xff30fe02
#define MONO_SAMPLE_EMULATOR_3X_4 0xff39fe02
#define MONO_SAMPLE_EMULATOR_3X_5 0x0030ffc3
#define RT_CONTROLS_SIZE 10
#define RT_CONTROLS_FS_SIZE 2

#define ESI_32_V3_DEF     "EMU SI-32 v3   \0"
#define EMULATOR_3X_DEF   "EMULATOR 3X    \0"

#define SAMPLE_EXT ".wav"

#define EMPTY_BANK "res/empty_bank"

struct emu3_bank
{
  char signature[SIGNATURE_SIZE];
  char name[NAME_SIZE];
  unsigned int parameters[BANK_PARAMETERS];
  char name_copy[NAME_SIZE];
  unsigned int more_parameters[MORE_BANK_PARAMETERS];
};

struct emu3_sample
{
  char name[NAME_SIZE];
  unsigned int parameters[SAMPLE_PARAMETERS];
  unsigned int sample_rate;
  unsigned int format;
  unsigned int more_parameters[MORE_SAMPLE_PARAMETERS];
  short int frames[];
};

struct emu3_preset_zone
{
  char parameters_a[12];
  unsigned char vcf_cutoff;
  char parameters_b[31];
  char vca_pan;
  char vcf_type_lfo_shape;
  char foo;
  char bar;
};

struct emu3_preset
{
  char name[NAME_SIZE];
  char rt_controls[RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE];
  char empty[16];
  char data[9];
  char nzones;
  char padding[0x58];
};

struct emu3_sample_descriptor
{
  short int *l_channel;
  short int *r_channel;
  struct emu3_sample *sample;
};

char *emu3_e3name_to_filename (const char *);

char *emu3_e3name_to_wav_filename (const char *);

char *emu3_wav_filename_to_e3name (const char *);

char *emu3_str_to_e3name (const char *);

void emu3_cpystr (char *, const char *);

int emu3_add_sample (char *, struct emu3_sample *, unsigned int, int);

int emu3_process_bank (const char *, int, char *, int, char *, int, int);

void emu3_print_sample_info (struct emu3_sample *, sf_count_t);

void emu3_print_preset_info (struct emu3_preset *);

int emu3_get_sample_channels (struct emu3_sample *);

void emu3_write_sample_file (struct emu3_sample *, sf_count_t);

int emu3_create_bank (const char *);

void emu3_init_sample_descriptor (struct emu3_sample_descriptor *,
				  struct emu3_sample *, int);

void emu3_write_frame (struct emu3_sample_descriptor *, short int[]);

void emu3_set_preset_rt_controls (struct emu3_preset *, char *);

void emu3_set_preset_cutoff (struct emu3_preset_zone *, int);

void emu3_set_preset_filter (struct emu3_preset_zone *, int);
