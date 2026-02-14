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

#define EMU3_SAMPLE_OPT_LOOP_MASK    0x000f
#define EMU3_SAMPLE_OPT_LOOP         0x0001
#define EMU3_SAMPLE_OPT_LOOP_RELEASE 0x0008
#define EMU3_SAMPLE_OPT_MONO_L       0x0020
#define EMU3_SAMPLE_OPT_MONO_R       0x0040
#define EMU3_SAMPLE_OPT_STEREO       (EMU3_SAMPLE_OPT_MONO_L | EMU3_SAMPLE_OPT_MONO_R)

#define EMU3_SAMPLE_HAS_CHANNEL_L(s) ((s)->options & EMU3_SAMPLE_OPT_MONO_L)

struct emu3_sample
{
  gchar name[EMU3_NAME_SIZE];
  guint32 header;
  guint32 start_l;		//always equals sizeof (struct emu3_sample)
  guint32 start_r;
  guint32 end_l;
  guint32 end_r;
  guint32 loop_start_l;
  guint32 loop_start_r;
  guint32 loop_end_l;
  guint32 loop_end_r;
  guint32 sample_rate;
  guint16 playback_rate;
  guint16 options;
  guint32 sample_data_offset_l;
  guint32 sample_data_offset_r;
  guint32 parameters[SAMPLE_PARAMETERS];
  gint16 frames[];
};

typedef enum emu3_ext_mode
{
  EMU3_EXT_MODE_NONE = 0,
  EMU3_EXT_MODE_NAME,
  EMU3_EXT_MODE_NAME_NUMBER
} emu3_ext_mode_t;

struct smpl_chunk_data
{
  guint32 manufacturer;
  guint32 product;
  guint32 sample_period;
  guint32 midi_unity_note;
  guint32 midi_pitch_fraction;
  guint32 smpte_format;
  guint32 smpte_offset;
  guint32 num_sample_loops;
  guint32 sample_data;
  struct sample_loop
  {
    guint32 cue_point_id;
    guint32 type;
    guint32 start;
    guint32 end;
    guint32 fraction;
    guint32 play_count;
  } sample_loop;
};

void emu3_process_sample (struct emu3_sample *sample, gint num,
			  emu3_ext_mode_t ext_mode, guint8 note,
			  gfloat fraction);

gint emu3_sample_get_smpl_chunk (SNDFILE * output,
				 struct smpl_chunk_data *smpl_chunk_data);

gint emu3_append_sample (struct emu_file *file, struct emu3_sample *sample,
			 const gchar * path, gint offset);

extern gint max_sample_rate;
extern gint bit_depth;

#endif
