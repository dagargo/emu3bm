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

#define SAMPLE_PARAMETERS 8
#define DEFAULT_SAMPLING_FREQ 44100

#define EMU3_SAMPLE_OPT_LOOP         0x00010000
#define EMU3_SAMPLE_OPT_LOOP_RELEASE 0x00080000
#define EMU3_SAMPLE_OPT_MONO_L       0x00200000
#define EMU3_SAMPLE_OPT_MONO_R       0x00400000
#define EMU3_SAMPLE_OPT_STEREO       (EMU3_SAMPLE_OPT_MONO_L | EMU3_SAMPLE_OPT_MONO_R)

#define EMU3_LOOP_START_FRAMES_TO_INT(loop_start) ((loop_start + 2) * sizeof (int16_t))
#define EMU3_LOOP_START_INT_TO_FRAMES(loop_start) ((loop_start / sizeof (int16_t)) - 2)

#define EMU3_LOOP_END_FRAMES_TO_INT(loop_end) ((loop_end + 1) * sizeof (int16_t))
#define EMU3_LOOP_END_INT_TO_FRAMES(loop_end) ((loop_end / sizeof (int16_t)) - 1)

#define EMU3_LOOP_POINT_INT_TO_BIN(x) (sizeof (struct emu3_sample) + x)
#define EMU3_LOOP_POINT_BIN_TO_INT(x) (x - sizeof (struct emu3_sample))

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
  uint32_t format;
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
  uint32_t num_sampler_loops;
  uint32_t sampler_data;
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

int emu3_get_sample_channels (struct emu3_sample *);

void emu3_process_sample (struct emu3_sample *sample, int num, int nframes,
			  emu3_ext_mode_t ext_mode);

int emu3_sample_get_smpl_chunk (SNDFILE * output,
				struct smpl_chunk_data *smpl_chunk_data);
#endif
