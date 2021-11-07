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

#ifndef SAMPLE_H
#define SAMPLE_H

#define NAME_SIZE 16

#define SAMPLE_PARAMETERS 9
#define MORE_SAMPLE_PARAMETERS 8
#define DEFAULT_SAMPLING_FREQ 44100

#define LOOP 0x00010000
#define LOOP_RELEASE 0x00080000

#define MONO_SAMPLE 0x00300001
#define MONO_SAMPLE_2 0x00500000
#define STEREO_SAMPLE 0x00700001
#define STEREO_SAMPLE_2 0x00700000
#define MONO_SAMPLE_3X 0x0030fe02

#define SAMPLE_EXT ".wav"

struct emu3_sample
{
  char name[NAME_SIZE];
  unsigned int parameters[SAMPLE_PARAMETERS];
  unsigned int sample_rate;
  unsigned int format;
  unsigned int more_parameters[MORE_SAMPLE_PARAMETERS];
  short int frames[];
};

int emu3_get_sample_channels (struct emu3_sample *);

void emu3_extract_sample (struct emu3_sample *, int, sf_count_t, int);

#endif
