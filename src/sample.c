/*
 *   sample.c
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

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "sample.h"

static char *
emu3_emu3name_to_filename (const char *objname)
{
  int i, size;
  const char *index = &objname[NAME_SIZE - 1];
  char *fname;

  for (size = NAME_SIZE; size > 0; size--)
    {
      if (*index != ' ')
	break;
      index--;
    }
  fname = (char *) malloc (size + 1);
  strncpy (fname, objname, size);
  fname[size] = '\0';
  for (i = 0; i < size; i++)
    if (fname[i] == '/' || fname[i] == 127)
      fname[i] = '?';

  return fname;
}

static char *
emu3_emu3name_to_wav_filename (const char *emu3name, int num, int ext_mode)
{
  char *fname = emu3_emu3name_to_filename (emu3name);
  char *wname = malloc (strlen (fname) + 9);

  if (ext_mode == 1)
    sprintf (wname, "%s%s", fname, SAMPLE_EXT);
  else
    sprintf (wname, "%03d-%s%s", num, fname, SAMPLE_EXT);

  free (fname);

  return wname;
}

int
emu3_get_sample_channels (struct emu3_sample *sample)
{
  if ((sample->format & STEREO_SAMPLE) == STEREO_SAMPLE)
    return 2;
  else if ((sample->format & MONO_SAMPLE_L) == MONO_SAMPLE_L
	   || (sample->format & MONO_SAMPLE_R) == MONO_SAMPLE_R)
    return 1;
  else
    return 1;
}

void
emu3_print_sample_info (struct emu3_sample *sample, sf_count_t nframes)
{
  for (int i = 0; i < SAMPLE_PARAMETERS; i++)
    emu_print (2, 1, "0x%08x ", sample->parameters[i]);
  emu_print (2, 1, "\n");
  emu_print (2, 1, "Sample format: 0x%08x\n", sample->format);
  emu_print (1, 1, "Channels: %d\n", emu3_get_sample_channels (sample));
  emu_print (1, 1, "Frames: %" PRId64 "\n", nframes);
  emu_print (1, 1, "Sample rate: %dHz\n", sample->sample_rate);
  emu_print (1, 1, "Loop enabled: %s\n",
	     sample->format & LOOP ? "on" : "off");
  emu_print (1, 1, "Loop in release: %s\n",
	     sample->format & LOOP_RELEASE ? "on" : "off");
  for (int i = 0; i < MORE_SAMPLE_PARAMETERS; i++)
    emu_print (2, 1, "0x%08x ", sample->more_parameters[i]);
  emu_print (2, 1, "\n");
}

void
emu3_extract_sample (struct emu3_sample *sample, int num, unsigned int len,
		     int ext_mode)
{
  SF_INFO sfinfo;
  SNDFILE *output;
  char *wav_file;
  short *l_channel, *r_channel;
  short frame[2];
  char *schannels;
  int channels = emu3_get_sample_channels (sample);

  //We divide between the bytes per frame (number of channels * 2 bytes)
  //The 16 or 8 bytes are the 4 or 8 short int used for padding.
  sf_count_t nframes =
    (len - sizeof (struct emu3_sample) - (8 * channels)) / (2 * channels);

  sfinfo.frames = nframes;
  sfinfo.samplerate = sample->sample_rate;
  sfinfo.channels = channels;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  wav_file = emu3_emu3name_to_wav_filename (sample->name, num, ext_mode);
  schannels = channels == 1 ? "mono" : "stereo";
  emu_debug (1, "Sample size: %d; frames: %d; channels: %s\n", len, nframes, schannels);
  emu3_print_sample_info (sample, nframes);
  emu_debug (1, "Extracting sample '%s'...\n", wav_file);

  output = sf_open (wav_file, SFM_WRITE, &sfinfo);
  l_channel = sample->frames + 2;
  if (channels == 2)
    r_channel = sample->frames + nframes + 6;
  for (int i = 0; i < nframes; i++)
    {
      frame[0] = *l_channel;
      l_channel++;
      if (channels == 2)
	{
	  frame[1] = *r_channel;
	  r_channel++;
	}
      if (!sf_writef_short (output, frame, 1))
	{
	  emu_error ("Error: %s\n", sf_strerror (output));
	  break;
	}
    }

  free (wav_file);
  sf_close (output);
}
