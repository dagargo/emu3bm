/*
 *   sample.h
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

#include <sndfile.h>
#include "utils.h"

#ifndef SAMPLE_H
#define SAMPLE_H

#define SAMPLE_PARAMETERS 6
#define MIN_SAMPLE_RATE 7000
#define MAX_SAMPLE_RATE 44100
#define MIN_BIT_DEPTH 2
#define MAX_BIT_DEPTH 16

#define EMU3_SAMPLE_OPT_LOOP         0x0001
#define EMU3_SAMPLE_OPT_LOOP_RELEASE 0x0008
#define EMU3_SAMPLE_OPT_MONO_L       0x0020
#define EMU3_SAMPLE_OPT_MONO_R       0x0040
#define EMU3_SAMPLE_OPT_STEREO       (EMU3_SAMPLE_OPT_MONO_L | EMU3_SAMPLE_OPT_MONO_R)

#define EMU3_SAMPLE_HAS_CHANNEL_L(s) ((s)->options & EMU3_SAMPLE_OPT_MONO_L)

struct emu3_sample
{
  char name[NAME_SIZE];
  uint32_t header;
  uint32_t start_l;		//always equals sizeof (struct emu3_sample)
  uint32_t start_r;
  uint32_t end_l;
  uint32_t end_r;
  uint32_t loop_start_l;
  uint32_t loop_start_r;
  uint32_t loop_end_l;
  uint32_t loop_end_r;
  uint32_t sample_rate;
  uint16_t playback_rate;
  uint16_t options;
  uint32_t sample_data_offset_l;
  uint32_t sample_data_offset_r;
  uint32_t parameters[SAMPLE_PARAMETERS];
  int16_t frames[];
};

typedef enum emu3_ext_mode
{
  EMU3_EXT_MODE_NONE = 0,
  EMU3_EXT_MODE_NAME,
  EMU3_EXT_MODE_NAME_NUMBER
} emu3_ext_mode_t;

struct smpl_chunk_data
{
  uint32_t manufacturer;
  uint32_t product;
  uint32_t sample_period;
  uint32_t midi_unity_note;
  uint32_t midi_pitch_fraction;
  uint32_t smpte_format;
  uint32_t smpte_offset;
  uint32_t num_sample_loops;
  uint32_t sample_data;
  struct sample_loop
  {
    uint32_t cue_point_id;
    uint32_t type;
    uint32_t start;
    uint32_t end;
    uint32_t fraction;
    uint32_t play_count;
  } sample_loop;
};

void emu3_process_sample (struct emu3_sample *sample, int num,
			  emu3_ext_mode_t ext_mode, uint8_t note,
			  float fraction);

int emu3_sample_get_smpl_chunk (SNDFILE * output,
				struct smpl_chunk_data *smpl_chunk_data);

int
emu3_append_sample (struct emu_file *file, struct emu3_sample *sample,
		    const char *path, int offset);

extern int max_sample_rate;
extern int bit_depth;

#endif
