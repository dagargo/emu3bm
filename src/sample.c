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

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "sample.h"

#define JUNK_CHUNK_ID "JUNK"
#define SMPL_CHUNK_ID "smpl"

static const uint8_t JUNK_CHUNK_DATA[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0
};

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

  if (ext_mode == EMU3_EXT_MODE_NAME_NUMBER)
    sprintf (wname, "%03d-%s%s", num, fname, SAMPLE_EXT);
  else
    sprintf (wname, "%s%s", fname, SAMPLE_EXT);

  free (fname);

  return wname;
}

int
emu3_get_sample_channels (struct emu3_sample *sample)
{
  if ((sample->format & EMU3_SAMPLE_OPT_STEREO) == EMU3_SAMPLE_OPT_STEREO)
    return 2;
  else if ((sample->format & EMU3_SAMPLE_OPT_MONO_L) == EMU3_SAMPLE_OPT_MONO_L
	   || (sample->format & EMU3_SAMPLE_OPT_MONO_R) ==
	   EMU3_SAMPLE_OPT_MONO_R)
    return 1;
  else
    return 1;
}

static void
emu3_print_sample_info (struct emu3_sample *sample, int num,
			sf_count_t nframes, int32_t *loop_start,
			uint32_t *loop_end)
{
  emu_print (0, 0, "Sample %03d: %.*s\n", num, NAME_SIZE, sample->name);

  if (sample->start_l != sizeof (struct emu3_sample))
    {
      emu_debug (1, "Unexpected constant: %d", sample->start_l);
    }

  emu_print (2, 1, "Sample header: 0x%08x\n", sample->header);
  emu_print (2, 1, "Start L: %d\n", sample->start_l);
  emu_print (2, 1, "Start R: %d\n", sample->start_r);
  emu_print (2, 1, "End   L: %d\n", sample->end_l);
  emu_print (2, 1, "End   R: %d\n", sample->end_r);
  emu_print (2, 1, "Loop start L: %d\n", sample->loop_start_l);
  emu_print (2, 1, "Loop start R: %d\n", sample->loop_start_r);
  emu_print (2, 1, "Loop end   L: %d\n", sample->loop_end_l);
  emu_print (2, 1, "Loop end:  R: %d\n", sample->loop_end_r);

  *loop_start = EMU3_LOOP_START_INT_TO_FRAMES (EMU3_LOOP_POINT_BIN_TO_INT
					       (sample->loop_start_l));
  *loop_end = EMU3_LOOP_END_INT_TO_FRAMES (EMU3_LOOP_POINT_BIN_TO_INT
					   (sample->loop_end_l));

  emu_print (2, 1, "Sample format: 0x%08x\n", sample->format);
  emu_print (1, 1, "Loop start: %d\n", *loop_start);
  emu_print (1, 1, "Loop end: %d\n", *loop_end);
  emu_print (1, 1, "Channels: %d\n", emu3_get_sample_channels (sample));
  emu_print (1, 1, "Frames: %d\n", nframes);
  emu_print (1, 1, "Sample rate: %d Hz\n", sample->sample_rate);
  emu_print (1, 1, "Loop enabled: %s\n",
	     sample->format & EMU3_SAMPLE_OPT_LOOP ? "on" : "off");
  emu_print (1, 1, "Loop in release: %s\n",
	     sample->format & EMU3_SAMPLE_OPT_LOOP_RELEASE ? "on" : "off");

  emu_print (2, 1, "Sample parameters:\n");
  for (int i = 0; i < SAMPLE_PARAMETERS; i++)
    emu_print (2, 2, "0x%08x (%d)\n", sample->parameters[i],
	       sample->parameters[i]);
}

void
emu3_process_sample (struct emu3_sample *sample, int num, int nframes,
		     emu3_ext_mode_t ext_mode)
{
  SF_INFO sfinfo;
  SNDFILE *output;
  char *wav_file;
  short *l_channel, *r_channel;
  short frame[2];
  uint32_t loop_start, loop_end;
  int channels = emu3_get_sample_channels (sample);
  struct SF_CHUNK_INFO smpl_chunk_info;
  struct SF_CHUNK_INFO junk_chunk_info;
  struct smpl_chunk_data smpl_chunk_data;

  emu3_print_sample_info (sample, num, nframes, &loop_start, &loop_end);

  if (!ext_mode)
    return;
  wav_file = emu3_emu3name_to_wav_filename (sample->name, num, ext_mode);

  emu_debug (1, "Extracting sample '%s'...", wav_file);

  sfinfo.frames = nframes;
  sfinfo.samplerate = sample->sample_rate;
  sfinfo.channels = channels;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  output = sf_open (wav_file, SFM_WRITE, &sfinfo);

  //The reason for writing this chunk is to make WAV files similar to the ones exported by Elektron Transfer.
  strcpy (junk_chunk_info.id, JUNK_CHUNK_ID);
  junk_chunk_info.id_size = strlen (JUNK_CHUNK_ID);
  junk_chunk_info.datalen = sizeof (JUNK_CHUNK_DATA);
  junk_chunk_info.data = (void *) JUNK_CHUNK_DATA;
  if (sf_set_chunk (output, &junk_chunk_info) != SF_ERR_NO_ERROR)
    {
      emu_error ("%s", sf_strerror (output));
    }

  smpl_chunk_data.manufacturer = 0;
  smpl_chunk_data.product = 0;
  smpl_chunk_data.sample_period = htole32 (1e9 / sample->sample_rate);
  smpl_chunk_data.midi_unity_note = 0;	//TODO: For any zone with this sample, get the original note.
  smpl_chunk_data.midi_pitch_fraction = 0;
  smpl_chunk_data.smpte_format = 0;
  smpl_chunk_data.smpte_offset = 0;
  smpl_chunk_data.num_sampler_loops = htole32 (1);
  smpl_chunk_data.sampler_data = 0;
  smpl_chunk_data.sample_loop.cue_point_id = 0;
  smpl_chunk_data.sample_loop.type = htole32 (sample->format & EMU3_SAMPLE_OPT_LOOP ? 0 : 0x7f);	// as in midi sds, 0x00 = forward loop, 0x7F = no loop
  smpl_chunk_data.sample_loop.start = htole32 (loop_start);
  smpl_chunk_data.sample_loop.end = htole32 (loop_end);
  smpl_chunk_data.sample_loop.fraction = 0;
  smpl_chunk_data.sample_loop.play_count = 0;

  strcpy (smpl_chunk_info.id, SMPL_CHUNK_ID);
  smpl_chunk_info.id_size = strlen (SMPL_CHUNK_ID);
  smpl_chunk_info.datalen = sizeof (struct smpl_chunk_data);
  smpl_chunk_info.data = &smpl_chunk_data;
  if (sf_set_chunk (output, &smpl_chunk_info) != SF_ERR_NO_ERROR)
    {
      emu_error ("%s", sf_strerror (output));
    }

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
	  emu_error ("%s", sf_strerror (output));
	  break;
	}
    }

  free (wav_file);
  sf_close (output);
}

int
emu3_sample_get_smpl_chunk (SNDFILE *input,
			    struct smpl_chunk_data *smpl_chunk_data)
{
  int loop_start, loop_end, loop;
  struct SF_CHUNK_INFO chunk_info;
  SF_CHUNK_ITERATOR *chunk_iter;

  loop = 0;
  strcpy (chunk_info.id, SMPL_CHUNK_ID);
  chunk_info.id_size = strlen (SMPL_CHUNK_ID);
  chunk_iter = sf_get_chunk_iterator (input, &chunk_info);

  if (chunk_iter)
    {
      loop = 1;
      chunk_info.datalen = sizeof (struct smpl_chunk_data);
      memset (smpl_chunk_data, 0, chunk_info.datalen);
      emu_debug (2, "%s chunk found (%d B)", SMPL_CHUNK_ID,
		 chunk_info.datalen);
      chunk_info.data = smpl_chunk_data;
      sf_get_chunk_data (chunk_iter, &chunk_info);

      while (chunk_iter)
	{
	  chunk_iter = sf_next_chunk_iterator (chunk_iter);
	}
    }

  return loop;
}
