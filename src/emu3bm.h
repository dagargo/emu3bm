/*
 *	emu3bm.h
 *	Copyright (C) 2016 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   emu3bm is free software: you can redistribute it and/or modify
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
#include <inttypes.h>
#include <stdarg.h>
#include <libgen.h>

#include "../config.h"

#define MEM_SIZE 0x08000000
#define FORMAT_SIZE 16
#define NAME_SIZE 16
#define BANK_PARAMETERS 5
#define MORE_BANK_PARAMETERS 3
#define SAMPLE_ADDR_START_EMU_3X 0x1bd2
#define SAMPLE_ADDR_START_EMU_THREE 0x204
#define PRESET_OFFSET_EMU_THREE 0x1a6fe
#define PRESET_START_EMU_3X 0x2b72
#define PRESET_START_EMU_THREE 0x74a
#define SAMPLE_OFFSET 0x400000	//This is also the max sample length in bytes
#define MAX_SAMPLES_EMU_3X 999
#define MAX_SAMPLES_EMU_THREE 99
#define PRESET_SIZE_ADDR_START_EMU_3X 0x17ca
#define PRESET_SIZE_ADDR_START_EMU_THREE 0x6c
#define MAX_PRESETS_EMU_3X 0x100
#define MAX_PRESETS_EMU_THREE 100
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
#define UNKNOWN_PARAMETERS_SIZE 16
#define NOTES 88		// 0x58

#define ESI_32_V3_DEF      "EMU SI-32 v3   \0"
#define EMULATOR_3X_DEF    "EMULATOR 3X    \0"
#define EMULATOR_THREE_DEF "EMULATOR THREE \0"

#define SAMPLE_EXT ".wav"

#define EMPTY_BANK "res/empty_bank"

#define emu3_log(level, indent, ...) {\
		if (level <= verbosity) { \
			for (int i = 0; i < indent; i++) \
				printf("\t"); \
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

struct emu3_bank
{
  char format[FORMAT_SIZE];
  char name[NAME_SIZE];
  unsigned int objects;
  unsigned int padding[4];
  unsigned int next;
  unsigned int parameters[BANK_PARAMETERS];
  char name_copy[NAME_SIZE];
  unsigned int selected_preset;
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

struct emu3_envelope
{
  unsigned char attack;
  unsigned char hold;
  unsigned char decay;
  unsigned char sustain;
  unsigned char release;
};

struct emu3_preset_zone_def
{
  unsigned char b1;
  unsigned char b2;
  unsigned char b3;
  unsigned char type;
};

struct emu3_preset_zone
{
  char root_note;
  unsigned char sample_id_lsb;
  unsigned char sample_id_msb;
  char parameter_a;
  struct emu3_envelope vca_envelope;
  char parameters_a2[2];
  unsigned char lfo_variation;
  unsigned char vcf_cutoff;
  unsigned char vcf_q;
  unsigned char vcf_envelope_amount;
  struct emu3_envelope vcf_envelope;
  struct emu3_envelope aux_envelope;
  char aux_envelope_amount;
  unsigned char aux_envelope_dest;
  char vel_to_vca_level;
  char vel_to_vca_attack;
  char vel_to_vcf_cutoff;
  char vel_to_pitch;
  char vel_to_aux_env;
  char vel_to_vcf_q;
  char vel_to_vcf_attack;
  char vel_to_sample_start;
  char vel_to_vca_pan;
  char lfo_to_pitch;
  char lfo_to_vca;
  char lfo_to_cutoff;
  char lfo_to_pan;
  char vca_level;
  char unknown_1;
  char unknown_2;
  char unknown_3;
  char vca_pan;
  unsigned char vcf_type_lfo_shape;
  char foo;			//0xff
  char bar;			///0x01
};

struct emu3_preset
{
  char name[NAME_SIZE];
  char rt_controls[RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE];
  char unknown_parameters[UNKNOWN_PARAMETERS_SIZE];
  char pitch_bend_range;
  char data[8];
  char nzones;
  unsigned char note_zone_mappings[NOTES];
};

struct emu3_sample_descriptor
{
  short int *l_channel;
  short int *r_channel;
  struct emu3_sample *sample;
};

struct emu3_file *emu3_open_file (const char *);

void emu3_close_file (struct emu3_file *);

void emu3_write_file (struct emu3_file *);

char *emu3_emu3name_to_filename (const char *);

char *emu3_emu3name_to_wav_filename (const char *);

char *emu3_wav_filename_to_emu3name (const char *);

char *emu3_str_to_emu3name (const char *);

void emu3_cpystr (char *, const char *);

int emu3_add_sample (struct emu3_file *, char *);

int emu3_extract_samples (struct emu3_file *);

int emu3_process_bank (struct emu3_file *, int, int, char *, int, int, int,
		       int, int);

void emu3_print_sample_info (struct emu3_sample *, sf_count_t);

void emu3_print_preset_info (struct emu3_preset *);

int emu3_get_sample_channels (struct emu3_sample *);

void emu3_write_sample_file (struct emu3_sample *, sf_count_t);

int emu3_create_bank (const char *);

void emu3_init_sample_descriptor (struct emu3_sample_descriptor *,
				  struct emu3_sample *, int);

void emu3_write_frame (struct emu3_sample_descriptor *, short int[]);

void emu3_set_preset_rt_controls (struct emu3_preset *, char *);

void emu3_set_preset_pbr (struct emu3_preset *, int);

void emu3_set_preset_zone_level (struct emu3_preset_zone *, int);

void emu3_set_preset_zone_cutoff (struct emu3_preset_zone *, int);

void emu3_set_preset_zone_q (struct emu3_preset_zone *, int);

void emu3_set_preset_zone_filter (struct emu3_preset_zone *, int);
