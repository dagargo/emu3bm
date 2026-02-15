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

#include <libgen.h>
#include <math.h>
#include <samplerate.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sample.h"

#define MINIMUM_LOOP_LEN 10

#define JUNK_CHUNK_ID "JUNK"
#define SMPL_CHUNK_ID "smpl"

gint max_sample_rate = MAX_SAMPLE_RATE;
gint bit_depth = MAX_BIT_DEPTH;

static const uint8_t JUNK_CHUNK_DATA[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0
};

struct emu3_sample_descriptor
{
  gint16 *l_channel;
  gint16 *r_channel;
  struct emu3_sample *sample;
};

static gchar *
emu3_emu3name_to_name (const gchar *objname)
{
  gint i, size;
  gchar *fname;
  const gchar *index = &objname[EMU3_NAME_SIZE - 1];

  for (size = EMU3_NAME_SIZE; size > 0; size--)
    {
      if (*index != ' ')
	break;
      index--;
    }

  fname = (gchar *) malloc (size + 1);
  if (size)
    {
      strncpy (fname, objname, size);
    }
  fname[size] = 0;

  for (i = 0; i < size; i++)
    if (fname[i] == '/' || fname[i] == 127)
      fname[i] = '?';

  return fname;
}

static gchar *
emu3_emu3name_to_wav_name (const gchar *emu3name, gint num, gint ext_mode)
{
  gchar *fname = emu3_emu3name_to_name (emu3name);
  gchar *wname = malloc (strlen (fname) + 9);

  if (ext_mode == EMU3_EXT_MODE_NAME_NUMBER)
    sprintf (wname, "%03d-%s%s", num, fname, SAMPLE_EXT);
  else
    sprintf (wname, "%s%s", fname, SAMPLE_EXT);

  free (fname);

  return wname;
}

static gint
emu3_get_sample_channels (struct emu3_sample *sample)
{
  if ((sample->options & EMU3_SAMPLE_OPT_STEREO) == EMU3_SAMPLE_OPT_STEREO)
    return 2;
  else if ((sample->options & EMU3_SAMPLE_OPT_MONO_L) ==
	   EMU3_SAMPLE_OPT_MONO_L
	   || (sample->options & EMU3_SAMPLE_OPT_MONO_R) ==
	   EMU3_SAMPLE_OPT_MONO_R)
    return 1;
  else
    return 1;
}

static guint16
emu3_playback_rate_to_bin (gint samplerate)
{
  gfloat f = -9799 + 1108 * logf (samplerate);
  return 0xf800 | ((gint) f);
}

static gint
emu3_playback_rate_from_bin (guint16 playback_rate, gint sample_rate)
{
  if (sample_rate == MAX_SAMPLE_RATE)
    {
      return MAX_SAMPLE_RATE;
    }
  else
    {
      gint v = playback_rate & ~0xf800;
      gfloat f = v + 9799;
      f /= 1108;
      f = expf (f);
      return f;
    }
}

static void
emu3_print_sample_info (struct emu3_sample *sample, gint num,
			guint32 *frames, guint32 *loop_start,
			guint32 *loop_end)
{
  guint32 sample_start, sample_end, sample_loop_start, sample_loop_end;

  //Some samples might have only the right channel.

  if (EMU3_SAMPLE_HAS_CHANNEL_L (sample))
    {
      sample_start = sample->start_l;
      sample_end = sample->end_l;
      sample_loop_start = sample->loop_start_l;
      sample_loop_end = sample->loop_end_l;
    }
  else
    {
      sample_start = sample->start_r;
      sample_end = sample->end_r;
      sample_loop_start = sample->loop_start_r;
      sample_loop_end = sample->loop_end_r;
    }

  if (sample_start != sizeof (struct emu3_sample))
    {
      emu_error ("Unexpected sample start: %d", sample_start);
    }

  *frames = ((sample_end + sizeof (gint16) -
	      sizeof (struct emu3_sample)) / sizeof (gint16));

  *loop_start =
    (sample_loop_start - sizeof (struct emu3_sample)) / sizeof (gint16);
  *loop_end =
    (sample_loop_end - sizeof (struct emu3_sample)) / sizeof (gint16);

  emu_print (0, 0, "Sample %03d: %.*s\n", num, EMU3_NAME_SIZE, sample->name);
  emu_print (1, 1, "Frames: %d\n", *frames);
  emu_print (1, 1, "Loop start: %d\n", *loop_start);
  emu_print (1, 1, "Loop end: %d\n", *loop_end);
  emu_print (1, 1, "Channels: %d\n", emu3_get_sample_channels (sample));
  emu_print (2, 1, "Start L: %d\n", sample->start_l);
  emu_print (2, 1, "Start R: %d\n", sample->start_r);
  emu_print (2, 1, "End   L: %d\n", sample->end_l);
  emu_print (2, 1, "End   R: %d\n", sample->end_r);
  emu_print (2, 1, "Loop start L: %d\n", sample->loop_start_l);
  emu_print (2, 1, "Loop start R: %d\n", sample->loop_start_r);
  emu_print (2, 1, "Loop end   L: %d\n", sample->loop_end_l);
  emu_print (2, 1, "Loop end   R: %d\n", sample->loop_end_r);
  emu_print (1, 1, "Sample   rate: %d Hz\n", sample->sample_rate);
  emu_print (1, 1, "Playback rate: %d Hz\n",
	     emu3_playback_rate_from_bin (sample->playback_rate,
					  sample->sample_rate));
  emu_print (1, 1, "Options: 0x%04x\n", sample->options);
  emu_print (1, 2, "Loop enabled: %s\n",
	     sample->options & EMU3_SAMPLE_OPT_LOOP ? "on" : "off");
  emu_print (1, 2, "Loop in release: %s\n",
	     sample->options & EMU3_SAMPLE_OPT_LOOP_RELEASE ? "on" : "off");
  emu_print (2, 1, "Header: 0x%08x\n", sample->header);
  emu_print (2, 1, "Sample data offset L: %d\n",
	     sample->sample_data_offset_l);
  emu_print (2, 1, "Sample data offset R: %d\n",
	     sample->sample_data_offset_r);

  emu_print (2, 1, "Sample parameters:\n");
  for (gint i = 0; i < SAMPLE_PARAMETERS; i++)
    emu_print (2, 2, "0x%08x (%d)\n", sample->parameters[i],
	       sample->parameters[i]);
}

void
emu3_process_sample (struct emu3_sample *sample, gint num,
		     emu3_ext_mode_t ext_mode, guint8 original_key,
		     gfloat tuning)
{
  SF_INFO sfinfo;
  SNDFILE *output;
  gchar *wav_file;
  gint16 *l_channel, *r_channel;
  gint16 frame[2];
  uint32_t frames, loop_start, loop_end;
  gint channels = emu3_get_sample_channels (sample);
  struct SF_CHUNK_INFO smpl_chunk_info;
  struct SF_CHUNK_INFO junk_chunk_info;
  struct smpl_chunk_data smpl_chunk_data;

  emu3_print_sample_info (sample, num, &frames, &loop_start, &loop_end);

  if (!ext_mode)
    return;

  wav_file = emu3_emu3name_to_wav_name (sample->name, num, ext_mode);

  emu_debug (1, "Extracting sample '%s'...", wav_file);

  sfinfo.frames = frames;
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

  while (tuning >= 100)
    {
      original_key++;
      tuning -= 100;
    }

  while (tuning < 0)
    {
      original_key--;
      tuning += 100;
    }

  smpl_chunk_data.manufacturer = 0;
  smpl_chunk_data.product = 0;
  smpl_chunk_data.sample_period = htole32 (1e9 / sample->sample_rate);
  smpl_chunk_data.midi_unity_note = htole32 (original_key + 21);	//Samplers use A-1 as note 0 but it's really an A0 when MIDI note 0 is C-1.
  smpl_chunk_data.midi_pitch_fraction =
    htole32 ((uint32_t) round (tuning * 256 / 100.0));
  smpl_chunk_data.smpte_format = 0;
  smpl_chunk_data.smpte_offset = 0;
  smpl_chunk_data.num_sample_loops = htole32 (1);
  smpl_chunk_data.sample_data = 0;

  smpl_chunk_data.sample_loop.cue_point_id = 0;
  smpl_chunk_data.sample_loop.type = htole32 (sample->options & EMU3_SAMPLE_OPT_LOOP ? 0 : 0x7f);	// as in midi sds, 0x00 = forward loop, 0x7F = no loop
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

  l_channel = sample->frames;
  if (channels == 2)
    r_channel = sample->frames + frames;
  else
    r_channel = l_channel;
  for (gint i = 0; i < frames; i++)
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

gint
emu3_sample_get_smpl_chunk (SNDFILE *input,
			    struct smpl_chunk_data *smpl_chunk_data)
{
  gint loop;
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

static void
emu3_sample_init_descriptor (struct emu3_sample_descriptor *sd,
			     struct emu3_sample *sample, gint frames)
{
  sd->sample = sample;
  sd->l_channel = sample->frames;
  if ((sample->options & EMU3_SAMPLE_OPT_STEREO) == EMU3_SAMPLE_OPT_STEREO)
    {
      sd->r_channel = sample->frames + frames;
    }
  else
    {
      // The initialization is required to avoid a warning.
      sd->r_channel = sample->frames;
    }
}

static void
emu3_write_frame (struct emu3_sample_descriptor *sd, gint16 frame[])
{
  struct emu3_sample *sample = sd->sample;
  *sd->l_channel = frame[0];
  sd->l_channel++;
  if ((sample->options & EMU3_SAMPLE_OPT_STEREO) == EMU3_SAMPLE_OPT_STEREO)
    {
      *sd->r_channel = frame[1];
      sd->r_channel++;
    }
}

// These additional fixes are needed by the ESI.
static void
emu3_check_and_fix_loop_point (guint32 *loop_point, guint32 frames)
{
  if (*loop_point < 6)
    {
      *loop_point = 6;
    }
  if (*loop_point > frames - 7)
    {
      *loop_point = frames - 7;
    }
}

void
emu3_sample_set_loop_start (struct emu3_sample *sample, gboolean mono,
			    guint32 frames, guint32 loop_start)
{
  emu3_check_and_fix_loop_point (&loop_start, frames);

  gint loop_start_bytes = loop_start * sizeof (gint16);
  sample->loop_start_l = loop_start_bytes + sizeof (struct emu3_sample);
  sample->loop_start_r = mono ? 0 :
    sample->loop_start_l + frames * sizeof (gint16);


  emu_debug (1, "Setting loop start at %d...", loop_start);
}

void
emu3_sample_set_loop_end (struct emu3_sample *sample, gboolean mono,
			  guint32 frames, guint32 loop_end)
{
  emu3_check_and_fix_loop_point (&loop_end, frames);

  emu_debug (1, "Setting loop end at %d...", loop_end);

  gint loop_end_bytes = loop_end * sizeof (gint16);
  sample->loop_end_l = loop_end_bytes + sizeof (struct emu3_sample);
  sample->loop_end_r = mono ? 0 :
    sample->loop_end_l + frames * sizeof (gint16);
}

static gint
emu3_sample_init (struct emu3_sample *sample, gint offset, gint samplerate,
		  gboolean mono, guint32 frames, guint32 loop_start,
		  guint32 loop_end, gint loop)
{
  gint mono_size, data_size, size;

  data_size = sizeof (gint16) * frames;
  mono_size = sizeof (struct emu3_sample) + data_size;
  size = mono_size + (mono ? 0 : data_size);

  sample->header = 0;
  sample->start_l = sizeof (struct emu3_sample);
  sample->start_r = mono ? 0 : mono_size;
  sample->end_l = mono_size - sizeof (gint16);
  sample->end_r = mono ? 0 : size - sizeof (gint16);

  emu3_sample_set_loop_start (sample, mono, frames, loop_start);
  emu3_sample_set_loop_end (sample, mono, frames, loop_end);

  sample->sample_rate = samplerate;

  if (samplerate < MAX_SAMPLE_RATE)
    {
      sample->playback_rate = emu3_playback_rate_to_bin (samplerate);
    }
  else
    {
      sample->playback_rate = 0;
    }

  sample->options |= mono ? EMU3_SAMPLE_OPT_MONO_L : EMU3_SAMPLE_OPT_STEREO;

  if (loop)
    {
      sample->options |= EMU3_SAMPLE_OPT_LOOP | EMU3_SAMPLE_OPT_LOOP_RELEASE;
    }

  sample->sample_data_offset_l = offset + sample->start_l;
  sample->sample_data_offset_r = mono ? 0 : offset + sample->start_r;

  for (gint i = 0; i < SAMPLE_PARAMETERS; i++)
    {
      sample->parameters[i] = 0;
    }

  return size;
}

static gint16 *
emu3_append_sample_get_data (SNDFILE *sndfile, SF_INFO *sfinfo,
			     gint *samplerate, guint32 *frames,
			     guint32 *loop_start, guint32 *loop_end,
			     gint *loop)
{
  gdouble ratio;
  gint direct_read;
  gint16 *output;
  gint smpl_chunk;
  gint total_gen_frames;
  struct smpl_chunk_data smpl_chunk_data;

  if (sfinfo->samplerate <= max_sample_rate)
    {
      direct_read = 1;
      *samplerate = sfinfo->samplerate;
      ratio = 1;
    }
  else
    {
      direct_read = 0;		//Requires resampling
      *samplerate = max_sample_rate;
      ratio = max_sample_rate / (gdouble) sfinfo->samplerate;
    }

  //Set scale factor. See http://www.mega-nerd.com/libsndfile/api.html#note2
  if ((sfinfo->format & SF_FORMAT_FLOAT) == SF_FORMAT_FLOAT ||
      (sfinfo->format & SF_FORMAT_DOUBLE) == SF_FORMAT_DOUBLE)
    {
      emu_debug (2,
		 "Setting scale factor to ensure correct integer readings...");
      sf_command (sndfile, SFC_SET_SCALE_FLOAT_INT_READ, NULL, SF_TRUE);
    }

  if (direct_read)
    {
      output = malloc (sizeof (gint16) * sfinfo->channels * sfinfo->frames);
      sf_readf_short (sndfile, output, sfinfo->frames);
      *frames = sfinfo->frames;
    }
  else
    {
      SRC_DATA srcdata;

      emu_debug (1, "Resampling...");

      srcdata.src_ratio = ratio;
      srcdata.input_frames = sfinfo->frames;
      srcdata.output_frames = ceil (ratio * sfinfo->frames);
      srcdata.data_in =
	malloc (sizeof (gfloat) * sfinfo->channels * sfinfo->frames);
      srcdata.data_out =
	malloc (sizeof (gfloat) * sfinfo->channels * srcdata.output_frames);

      sf_readf_float (sndfile, (gfloat *) srcdata.data_in, sfinfo->frames);

      gint err = src_simple (&srcdata, SRC_SINC_BEST_QUALITY,
			     sfinfo->channels);
      if (err)
	{
	  emu_error ("Error while resampling: %s", src_strerror (err));
	  free ((gfloat *) srcdata.data_in);
	  free (srcdata.data_out);
	  return NULL;
	}

      emu_debug (1,
		 "Resampling done. Used frames: %ld; generated frames: %ld",
		 srcdata.input_frames_used, srcdata.output_frames_gen);

      total_gen_frames = sfinfo->channels * srcdata.output_frames_gen;
      output = malloc (sizeof (gint16) * total_gen_frames);
      src_float_to_short_array (srcdata.data_out, output, total_gen_frames);

      if (bit_depth < MAX_BIT_DEPTH)
	{
	  guint16 *v = (guint16 *) output;
	  guint16 mask = 0x8000;
	  for (gint i = 1; i < bit_depth; i++)
	    {
	      mask = 0x8000 | (mask >> 1);
	    }

	  emu_debug (1, "Using bit mask '0x%4x'", mask);

	  for (gint i = 0; i < total_gen_frames; i++, v++)
	    {
	      *v = (*v & mask);
	    }
	}

      free ((gfloat *) srcdata.data_in);
      free (srcdata.data_out);

      *frames = srcdata.output_frames_gen;

      // Sometimes libsamplerate returns less frames less than expected.
      // This fixes the ratio, which is used to calculate the loop points.
      ratio = srcdata.output_frames_gen / (gdouble) sfinfo->frames;
    }

  smpl_chunk = emu3_sample_get_smpl_chunk (sndfile, &smpl_chunk_data);
  if (smpl_chunk)
    {
      *loop = smpl_chunk_data.sample_loop.type == htole32 (0x7f) ? 0 : 1;

      *loop_start = smpl_chunk_data.sample_loop.start * ratio;
      if (*loop_start >= *frames)
	{
	  emu_error ("Bad loop start. Using sample start...");
	  *loop_start = 0;
	}

      *loop_end = smpl_chunk_data.sample_loop.end * ratio;
      if (*loop_end >= *frames)
	{
	  emu_error ("Bad loop end. Using sample end...");
	  *loop_end = *frames - 1;
	}

      // As loops can't be less than MINIMUM_LOOP_LEN samples, it's better to ignore the loop.
      if (*loop_start == *loop_end)
	{
	  *loop = 0;
	}
    }
  else
    {
      // Other tools would've enabled the loop and set both loop points to the last sample.
      // Not only is this not desirable here because the loop can be disabled but also because
      // the loop can't be smaller than 10 samples (MINIMUM_LOOP_LEN), which would very likely
      // create audio artifacts. However, when exporting the sample, the result will be
      // different compared to the aforementioned tools.
      *loop = 0;
      *loop_start = 0;
      *loop_end = *frames - 1;
    }

  //This fixes some "Mono End Loop!!!  0006" errors when editing the loop points in the sampler.
  if (*loop_end - *loop_start < MINIMUM_LOOP_LEN)
    {
      *loop_start = *loop_end - MINIMUM_LOOP_LEN;
    }

  emu_debug (1, "Loop: %s; loop start at %d; loop end at %d",
	     *loop ? "on" : "off", *loop_start, *loop_end);

  return output;
}

//returns the sample size in bytes that the the sample takes in the bank
gint
emu3_append_sample (struct emu_file *file, struct emu3_sample *sample,
		    const gchar *path, gint offset, gboolean *mono,
		    guint32 *frames)
{
  SF_INFO sfinfo;
  SNDFILE *sndfile;
  gint16 *f, *data = NULL;
  gint16 zero[2] = { 0, 0 };
  const gchar *filename;
  gint loop, size, samplerate;
  guint32 loop_start, loop_end;
  struct emu3_sample_descriptor sd;

  if (access (path, R_OK) != 0)
    {
      emu_error ("Can't open sample '%s'", path);
      return -1;
    }

  size = -1;
  sfinfo.format = 0;
  sndfile = sf_open (path, SFM_READ, &sfinfo);

  if (sfinfo.channels > 2)
    {
      emu_error ("Sample neither mono nor stereo");
      goto close;
    }

  data = emu3_append_sample_get_data (sndfile, &sfinfo, &samplerate, frames,
				      &loop_start, &loop_end, &loop);
  if (!data)
    {
      goto close;
    }

  *mono = sfinfo.channels == 1;
  size = emu3_sample_init (sample, offset, samplerate, *mono, *frames,
			   loop_start, loop_end, loop);
  if (file->size + size > EMU3_MEM_SIZE)
    {
      emu_error ("Bank is full");
      size = -1;
      goto close;
    }

  gchar *basec = strdup (path);
  filename = basename (basec);
  emu_print (0, 0, "Appending sample '%s' (%d frames, %d channels)...\n",
	     filename, *frames, sfinfo.channels);
  //Sample header initialization
  gchar *name = emu_filename_to_filename_wo_ext (filename, NULL);
  gchar *emu3name = emu3_str_to_emu3name (name);
  emu3_cpystr (sample->name, emu3name);

  free (basec);
  free (name);
  free (emu3name);

  emu3_sample_init_descriptor (&sd, sample, *frames);

  f = data;
  for (gint i = 0; i < *frames; i++, f += sfinfo.channels)
    {
      // First 2 and last 2 frames must be set to 0.
      // Without this, the sampler will complain with a "Mono Start Zero!!!001" error message.
      // As indicated in the manual, running the sample integrity in the digital tools menu would fix the error and it'll fix it by doing this.
      // In previous versions of emu3bm, the pairs of 0 samples were appended automatically but this broke SDS compatibility frame count.
      if (i < 2 || i >= *frames - 2)
	{
	  emu3_write_frame (&sd, zero);
	}
      else
	{
	  emu3_write_frame (&sd, f);
	}
    }

  emu_debug (1, "Appended %d B (0x%08x B)", size, size);

close:
  free (data);
  sf_close (sndfile);

  return size;
}
