/*
 *	emu3bm.c
 *	Copyright (C) 2018 David García Goñi <dagargo at gmail dot com>
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

#include "emu3bm.h"

extern int verbosity;
extern char *note_names[];

char DEFAULT_RT_CONTROLS[RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE] =
  { 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 8 };

const char *RT_CONTROLS_SRC[] = {
  "Pitch Control",
  "Mod Control",
  "Pressure Control",
  "Pedal Control",
  "MIDI A Control",
  "MIDI B Control"
};

const int RT_CONTROLS_SRC_SIZE = sizeof (RT_CONTROLS_SRC) / sizeof (char *);

const char *RT_CONTROLS_DST[] = {
  "Off",
  "Pitch",
  "VCF Cutoff",
  "VCA Level",
  "LFO -> Pitch",
  "LFO -> Cutoff",
  "LFO -> VCA",
  "Pan",
  "Attack",
  "Crossfade",
  "VCF NoteOn Q"
};

const int RT_CONTROLS_DST_SIZE = sizeof (RT_CONTROLS_DST) / sizeof (char *);

const char *RT_CONTROLS_FS_SRC[] = {
  "Footswitch 1",
  "Footswitch 2",
};

const int RT_CONTROLS_FS_SRC_SIZE =
  sizeof (RT_CONTROLS_FS_SRC) / sizeof (char *);

const char *RT_CONTROLS_FS_DST[] = {
  "Off",
  "Sustain",
  "Cross-Switch",
  "Unused 1",
  "Unused 2",
  "Unused 3",
  "Unused A",
  "Unused B",
  "Preset Increment",
  "Preset Decrement"
};

const int RT_CONTROLS_FS_DST_SIZE =
  sizeof (RT_CONTROLS_FS_DST) / sizeof (char *);

const char *LFO_SHAPE[] = {
  "triangle",
  "sine",
  "sawtooth",
  "square"
};

const char *VCF_TYPE[] = {
  "2 Pole Lowpass",
  "4 Pole Lowpass",
  "6 Pole Lowpass",
  "2nd Ord Hipass",
  "4th Ord Hipass",
  "2nd O Bandpass",
  "4th O Bandpass",
  "Contrary BandP",
  "Swept EQ 1 oct",
  "Swept EQ 2->1",
  "Swept EQ 3->1",
  "Phaser 1",
  "Phaser 2",
  "Bat-Phaser",
  "Flanger Lite",
  "Vocal Ah-Ay-Ee",
  "Vocal Oo-Ah",
  "Bottom Feeder",
  "ESi/E3x Lopass",
  "Unknown"
};

const int VCF_TYPE_SIZE = sizeof (VCF_TYPE) / sizeof (char *);

char *
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
    if (fname[i] == '/')
      fname[i] = '?';

  return fname;
}

char *
emu3_emu3name_to_wav_filename (const char *emu3name, int num)
{
  char *fname = emu3_emu3name_to_filename (emu3name);
  char *wname = malloc (strlen (fname) + 9);

  sprintf(wname, "%03d-%s%s", num, fname, SAMPLE_EXT);
  return wname;
}

char *
emu3_wav_filename_to_filename (const char *wav_file)
{
  char *filename = malloc (strlen (wav_file) + 1);

  strcpy (filename, wav_file);
  char *ext = strrchr (wav_file, '.');
  if (strcmp (ext, SAMPLE_EXT) == 0)
    {
      free (filename);
      size_t len_wo_ext = strlen (wav_file) - strlen (SAMPLE_EXT);
      filename = malloc (len_wo_ext + 1);
      strncpy (filename, wav_file, len_wo_ext);
      filename[len_wo_ext] = '\0';
    }
  return filename;
}

char *
emu3_str_to_emu3name (const char *src)
{
  size_t len = strlen (src);
  if (len > NAME_SIZE)
    len = NAME_SIZE;

  char *emu3name = malloc (len + 1);
  strncpy (emu3name, src, len);
  emu3name[len] = '\0';

  char *c = emu3name;
  for (int i = 0; i < len; i++, c++)
    if (!isalnum (*c) && *c != ' ' && *c != '#' && *c != '-')
      *c = '?';

  return emu3name;
}

void
emu3_cpystr (char *dst, const char *src)
{
  int len = strlen (src);

  memcpy (dst, src, NAME_SIZE);
  memset (&dst[len], ' ', NAME_SIZE - len);
}


int
emu3_get_sample_channels (struct emu3_sample *sample)
{
  if ((sample->format & STEREO_SAMPLE) == STEREO_SAMPLE
      || (sample->format & STEREO_SAMPLE_2) == STEREO_SAMPLE_2)
    return 2;
  else if ((sample->format & MONO_SAMPLE) == MONO_SAMPLE
	   || (sample->format & MONO_SAMPLE_2) == MONO_SAMPLE_2
	   || (sample->format & MONO_SAMPLE_3X) == MONO_SAMPLE_3X)
    return 1;
  else
    return 1;
}

void
emu3_print_sample_info (struct emu3_sample *sample, sf_count_t nframes)
{
  for (int i = 0; i < SAMPLE_PARAMETERS; i++)
    emu3_log (2, 1, "0x%08x ", sample->parameters[i]);
  emu3_log (2, 1, "\n");
  emu3_log (2, 1, "Sample format: 0x%08x\n", sample->format);
  emu3_log (1, 1, "Channels: %d\n", emu3_get_sample_channels (sample));
  emu3_log (1, 1, "Frames: %" PRId64 "\n", nframes);
  emu3_log (1, 1, "Sample rate: %dHz\n", sample->sample_rate);
  emu3_log (1, 1, "Loop enabled: %s\n", sample->format & LOOP ? "on" : "off");
  emu3_log (1, 1, "Loop in release: %s\n",
	    sample->format & LOOP_RELEASE ? "on" : "off");
}

//Level: [0, 0x7f] -> [0, 100]
int
emu3_get_percent_value (char value)
{
  return (int) (value * 100 / 127.0);
}

//Pan: [0, 0x80] -> [-100, +100]
int
emu3_get_vca_pan (char vca_pan)
{
  return (int) ((vca_pan - 0x40) * 1.5625);
}

void
emu3_print_zone_info (struct emu3_preset_zone *zone)
{
  emu3_log (1, 3, "Sample: %d\n",
	    (zone->sample_id_msb << 8) + zone->sample_id_lsb);
  emu3_log (1, 3, "Original note: %s\n", note_names[zone->root_note]);
  emu3_log (1, 3, "VCA level: %d\n",
	    emu3_get_percent_value (zone->vca_level));
  emu3_log (1, 3, "VCA pan: %d\n", emu3_get_vca_pan (zone->vca_pan));

  emu3_log (1, 3,
	    "VCA envelope: attack: %d, hold: %d, decay: %d, sustain: %d\%, release: %d.\n",
	    zone->vca_envelope.attack, zone->vca_envelope.hold,
	    zone->vca_envelope.decay,
	    emu3_get_percent_value (zone->vca_envelope.sustain),
	    zone->vca_envelope.release);

  //Filter type might only work with ESI banks
  int vcf_type = zone->vcf_type_lfo_shape >> 3;
  if (vcf_type > VCF_TYPE_SIZE - 1)
    vcf_type = VCF_TYPE_SIZE - 1;
  emu3_log (1, 3, "VCF type: %s\n", VCF_TYPE[vcf_type]);

  //Cutoff: [0, 255] -> [26, 74040]
  int cutoff = zone->vcf_cutoff;
  emu3_log (1, 3, "VCF cutoff: %d\n", cutoff);

  //Filter Q might only work with ESI banks
  //ESI Q: [0x80, 0xff] -> [0, 100]
  //Other formats: [0, 0x7f]
  int q = (zone->vcf_q - 0x80) * 100 / 127;

  emu3_log (1, 3, "VCF Q: %d\n", q);

  emu3_log (1, 3, "VCF envelope amount: %d\n",
	    emu3_get_percent_value (zone->vcf_envelope_amount));

  emu3_log (1, 3,
	    "VCF envelope: attack: %d, hold: %d, decay: %d, sustain: %d\%, release: %d.\n",
	    zone->vcf_envelope.attack, zone->vcf_envelope.hold,
	    zone->vcf_envelope.decay,
	    emu3_get_percent_value (zone->vcf_envelope.sustain),
	    zone->vcf_envelope.release);

  emu3_log (1, 3, "Auxiliary envelope amount: %d\n",
	    emu3_get_percent_value (zone->aux_envelope_amount));

  emu3_log (1, 3,
	    "Auxiliary envelope: attack: %d, hold: %d, decay: %d, sustain: %d\%, release: %d.\n",
	    zone->aux_envelope.attack, zone->aux_envelope.hold,
	    zone->aux_envelope.decay,
	    emu3_get_percent_value (zone->aux_envelope.sustain),
	    zone->aux_envelope.release);

  emu3_log (1, 3, "Velocity to Pitch: %d\n", zone->vel_to_pitch);
  emu3_log (1, 3, "Velocity to VCA Level: %d\n",
	    emu3_get_percent_value (zone->vel_to_vca_level));
  emu3_log (1, 3, "Velocity to VCA Attack: %d\n", zone->vel_to_vca_attack);
  emu3_log (1, 3, "Velocity to VCF Cutoff: %d\n", zone->vel_to_vcf_cutoff);
  emu3_log (1, 3, "Velocity to VCF Q: %d\n", zone->vel_to_vcf_q);
  emu3_log (1, 3, "Velocity to VCF Attack: %d\n", zone->vel_to_vcf_attack);
  emu3_log (1, 3, "Velocity to Pan: %d\n",
	    emu3_get_percent_value (zone->vel_to_vca_pan));
  emu3_log (1, 3, "Velocity to Sample Start: %d\n",
	    zone->vel_to_sample_start);
  emu3_log (1, 3, "Velocity to Auxiliary Env: %d\n", zone->vel_to_aux_env);

  emu3_log (1, 3, "LFO shape: %s\n",
	    LFO_SHAPE[zone->vcf_type_lfo_shape & 0x3]);
  emu3_log (1, 3, "LFO->Pitch: %d\n",
	    emu3_get_percent_value (zone->lfo_to_pitch));
  emu3_log (1, 3, "LFO->Cutoff: %d\n",
	    emu3_get_percent_value (zone->lfo_to_cutoff));
  emu3_log (1, 3, "LFO->VCA: %d\n",
	    emu3_get_percent_value (zone->lfo_to_vca));
  emu3_log (1, 3, "LFO->Pan: %d\n",
	    emu3_get_percent_value (zone->lfo_to_pan));
}

void
emu3_print_preset_info (struct emu3_preset *preset)
{
  for (int i = 0; i < RT_CONTROLS_SRC_SIZE; i++)
    {
      int dst = 0;
      for (int j = 0; j < RT_CONTROLS_SIZE; j++)
	{
	  if (preset->rt_controls[j] == i + 1)
	    {
	      dst = j + 1;
	      break;
	    }
	}
      emu3_log (1, 1, "Mapping: %s - %s\n", RT_CONTROLS_SRC[i],
		RT_CONTROLS_DST[dst]);
    }
  for (int i = 0; i < RT_CONTROLS_FS_SIZE; i++)
    {
      emu3_log (1, 1, "Mapping: %s - %s\n",
		RT_CONTROLS_FS_SRC[i],
		RT_CONTROLS_FS_DST[preset->rt_controls
				   [RT_CONTROLS_SIZE + i]]);
    }
  for (int i = 0; i < PRESET_UNKNOWN_0_SIZE; i++)
    emu3_log (2, 1, "Unknown_0 %d: %d\n", i, preset->unknown_0[i]);
  emu3_log (1, 1, "Pitch Bend Range: %d\n", preset->pitch_bend_range);
}

void
emu3_set_preset_rt_control (struct emu3_preset *preset, int src, int dst)
{
  if (dst >= 0 && dst < RT_CONTROLS_DST_SIZE)
    {
      emu3_log (0, 1, "Setting controller %s to %s...\n",
		RT_CONTROLS_SRC[src], RT_CONTROLS_DST[dst]);
      if (dst >= 0)
	{
	  for (int i = 0; i < RT_CONTROLS_SIZE; i++)
	    if (preset->rt_controls[i] == src + 1)
	      preset->rt_controls[i] = 0;
	  if (dst > 0)
	    preset->rt_controls[dst - 1] = src + 1;
	}
    }
  else
    fprintf (stderr, "Invalid destination %d for %s\n", dst,
	     RT_CONTROLS_SRC[src]);
}

void
emu3_set_preset_rt_control_fs (struct emu3_preset *preset, int src, int dst)
{
  if (dst >= 0 && dst < RT_CONTROLS_FS_DST_SIZE)
    {
      emu3_log (0, 1, "Setting controller %s to %s...\n",
		RT_CONTROLS_FS_SRC[src], RT_CONTROLS_FS_DST[dst]);
      preset->rt_controls[src + RT_CONTROLS_FS_DST_SIZE] = dst;
    }
  else
    fprintf (stderr, "Invalid destination %d for %s\n", dst,
	     RT_CONTROLS_FS_SRC[src]);
}

void
emu3_set_preset_zone_level (struct emu3_preset_zone *zone, int level)
{
  if (level < 0 || level > 100)
    fprintf (stderr, "Value %d for level not in range [0, 100]\n", level);
  else
    {
      emu3_log (0, 1, "Setting level to %d...\n", level);
      zone->vca_level = (unsigned char) (level * 127 / 100);
    }
}

void
emu3_set_preset_zone_cutoff (struct emu3_preset_zone *zone, int cutoff)
{
  if (cutoff < 0 || cutoff > 255)
    fprintf (stderr, "Value for cutoff %d not in range [0, 255]\n", cutoff);
  else
    {
      emu3_log (0, 1, "Setting cutoff to %d...\n", cutoff);
      zone->vcf_cutoff = (unsigned char) cutoff;
    }
}

void
emu3_set_preset_zone_q (struct emu3_preset_zone *zone, int q)
{
  if (q < 0 || q > 100)
    fprintf (stderr, "Value %d for Q not in range [0, 100]\n", q);
  else
    {
      emu3_log (0, 1, "Setting Q to %d...\n", q);
      zone->vcf_q = (unsigned char) ((q * 127 / 100) + 0x80);
    }
}

void
emu3_set_preset_zone_filter (struct emu3_preset_zone *zone, int filter)
{
  if (filter < 0 || filter > VCF_TYPE_SIZE - 2)
    fprintf (stderr, "Value %d for filter not in range [0, %d]\n",
	     filter, VCF_TYPE_SIZE - 2);
  else
    {
      emu3_log (0, 1, "Setting filter to %s...\n", VCF_TYPE[filter]);
      zone->vcf_type_lfo_shape =
	((unsigned char) filter) << 3 | zone->vcf_type_lfo_shape & 0x3;
    }
}

void
emu3_set_preset_rt_controls (struct emu3_preset *preset, char *rt_controls)
{
  char *token;
  char *endtoken;
  int i;
  int controller;

  emu3_log (0, 1, "Setting realtime controls...\n");
  i = 0;
  while (i < RT_CONTROLS_SIZE && (token = strsep (&rt_controls, ",")) != NULL)
    {
      if (*token == '\0')
	fprintf (stderr, "Empty value\n");
      else
	{
	  controller = strtol (token, &endtoken, 10);
	  if (*endtoken == '\0')
	    {
	      if (i < RT_CONTROLS_SRC_SIZE)
		emu3_set_preset_rt_control (preset, i, controller);
	      else if (i < RT_CONTROLS_SRC_SIZE + RT_CONTROLS_FS_SRC_SIZE)
		{
		  emu3_set_preset_rt_control_fs (preset,
						 i - RT_CONTROLS_SRC_SIZE,
						 controller);
		}
	      else
		fprintf (stderr, "Too many controls\n");
	    }
	  else
	    fprintf (stderr, "'%s' not a number\n", token);
	}
      i++;
    }
}

void
emu3_set_preset_pbr (struct emu3_preset *preset, int pbr)
{
  preset->pitch_bend_range = pbr;
}

void
emu3_init_sample_descriptor (struct emu3_sample_descriptor *sd,
			     struct emu3_sample *sample, int frames)
{
  sd->sample = sample;

  sd->l_channel = sample->frames;
  if ((sample->format & STEREO_SAMPLE) == STEREO_SAMPLE)
    //We consider the 4 shorts padding that the left channel has
    sd->r_channel = sample->frames + frames + 4;
}

void
emu3_write_frame (struct emu3_sample_descriptor *sd, short int frame[])
{
  struct emu3_sample *sample = sd->sample;
  *sd->l_channel = frame[0];
  sd->l_channel++;
  if ((sample->format & STEREO_SAMPLE) == STEREO_SAMPLE)
    {
      *sd->r_channel = frame[1];
      sd->r_channel++;
    }
}

//returns the sample size in bytes that the the sample takes in the bank
int
emu3_append_sample (char *path, struct emu3_sample *sample, int loop)
{
  SF_INFO input_info;
  SNDFILE *input;
  int size = 0;
  unsigned int data_size;
  short int frame[2];
  short int zero[] = { 0, 0 };
  int mono_size;
  const char *filename;
  struct emu3_sample_descriptor sd;

  input_info.format = 0;
  input = sf_open (path, SFM_READ, &input_info);
  //TODO: add more formats
  if ((input_info.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
    fprintf (stderr, "Sample not in a valid format. Skipping...\n");
  else if (input_info.channels > 2)
    fprintf (stderr, "Sample neither mono nor stereo. Skipping...\n");
  else
    {
      char *basec = strdup (path);
      filename = basename (basec);
      emu3_log (0, 0,
		"Appending sample %s... (%" PRId64
		" frames, %d channels)\n", filename, input_info.frames,
		input_info.channels);
      //Sample header initialization
      char *name = emu3_wav_filename_to_filename (filename);
      char *emu3name = emu3_str_to_emu3name (name);
      emu3_cpystr (sample->name, emu3name);

      free (basec);
      free (name);
      free (emu3name);

      data_size = sizeof (short int) * (input_info.frames + 4);
      mono_size = sizeof (struct emu3_sample) + data_size;
      size = mono_size + (input_info.channels == 1 ? 0 : data_size);
      sample->parameters[0] = 0;
      //Start of left channel
      sample->parameters[1] = sizeof (struct emu3_sample);
      //Start of right channel
      sample->parameters[2] = input_info.channels == 1 ? 0 : mono_size;
      //Last sample of left channel
      sample->parameters[3] = mono_size - 2;
      //Last sample of right channel
      sample->parameters[4] = input_info.channels == 1 ? 0 : size - 2;

      int loop_start = 4;	//This is a constant
      //Example (mono and stereo): Loop start @ 9290 sample is stored as ((9290 + 2) * 2) + sizeof (struct emu3_sample)
      sample->parameters[5] =
	((loop_start + 2) * 2) + sizeof (struct emu3_sample);
      //Example
      //Mono: Loop start @ 9290 sample is stored as (9290 + 2) * 2
      //Stereo: Frames * 2 + parameters[5] + 8
      sample->parameters[6] =
	input_info.channels ==
	1 ? (loop_start + 2) * 2 : input_info.frames * 2 +
	sample->parameters[5] + 8;

      int loop_end = input_info.frames - 10;	//This is a constant
      //Example (mono and stereo): Loop end @ 94008 sample is stored as ((94008 + 1) * 2) + sizeof (struct emu3_sample)
      sample->parameters[7] =
	((loop_end + 1) * 2) + sizeof (struct emu3_sample);
      //Example
      //Mono: Loop end @ 94008 sample is stored as ((94008 + 1) * 2)
      //Stereo: Frames * 2 + parameters[7] + 8
      sample->parameters[8] =
	input_info.channels ==
	1 ? (loop_end + 1) * 2 : input_info.frames * 2 +
	sample->parameters[7] + 8;

      sample->sample_rate = DEFAULT_SAMPLING_FREQ;

      sample->format = input_info.channels == 1 ? MONO_SAMPLE : STEREO_SAMPLE;

      if (loop)
	sample->format = sample->format | LOOP | LOOP_RELEASE;

      for (int i = 0; i < MORE_SAMPLE_PARAMETERS; i++)
	sample->more_parameters[i] = 0;

      emu3_init_sample_descriptor (&sd, sample, input_info.frames);

      //2 first frames set to 0
      emu3_write_frame (&sd, zero);
      emu3_write_frame (&sd, zero);

      for (int i = 0; i < input_info.frames; i++)
	{
	  sf_readf_short (input, frame, 1);
	  emu3_write_frame (&sd, frame);
	}

      //2 end frames set to 0
      emu3_write_frame (&sd, zero);
      emu3_write_frame (&sd, zero);

      emu3_log (1, 1, "Appended %dB (0x%08x).\n", size, size);
    }
  sf_close (input);

  return size;
}

void
emu3_extract_sample (struct emu3_sample *sample, int num, sf_count_t nframes)
{
  SF_INFO output_info;
  SNDFILE *output;
  char *wav_file;
  short *l_channel, *r_channel;
  short frame[2];
  char *schannels;
  int channels = emu3_get_sample_channels (sample);
  int samplerate = sample->sample_rate;

  output_info.frames = nframes;
  output_info.samplerate = samplerate;
  output_info.channels = channels;
  output_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  wav_file = emu3_emu3name_to_wav_filename (sample->name, num);
  schannels = channels == 1 ? "mono" : "stereo";
  emu3_log (0, 1, "Extracting %s sample %s...\n", schannels, wav_file);

  output = sf_open (wav_file, SFM_WRITE, &output_info);
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
	  fprintf (stderr, "Error: %s\n", sf_strerror (output));
	  break;
	}
    }
  sf_close (output);
}

unsigned int
emu3_get_preset_address (struct emu3_bank *bank, int preset)
{
  unsigned int offset = emu3_get_preset_addresses (bank)[preset];

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return PRESET_START_EMU_3X + offset;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return PRESET_START_EMU_THREE + offset - PRESET_OFFSET_EMU_THREE;
  else
    return 0;
}

unsigned int
emu3_get_sample_start_address (struct emu3_bank *bank)
{
  int max_presets = emu3_get_max_presets (bank);
  unsigned int *paddresses = emu3_get_preset_addresses (bank);
  unsigned int offset = paddresses[max_presets];
  unsigned int start_address;

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    //There is always a 0xee byte before the samples
    start_address = PRESET_START_EMU_3X + 1;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    //There is always a 0x00 byte before the samples
    start_address = PRESET_START_EMU_THREE + 1 - PRESET_OFFSET_EMU_THREE;
  else
    start_address = 0;

  return start_address + offset;
}

unsigned int *
emu3_get_sample_addresses (struct emu3_bank *bank)
{
  char *raw = (char *) bank;

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return (unsigned int *) &(raw[SAMPLE_ADDR_START_EMU_3X]);
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return (unsigned int *) &(raw[SAMPLE_ADDR_START_EMU_THREE]);
  else
    return NULL;
}

unsigned int
emu3_get_next_sample_address (struct emu3_bank *bank)
{
  int max_samples = emu3_get_max_samples (bank);
  unsigned int *saddresses = emu3_get_sample_addresses (bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (bank);

  return sample_start_addr + saddresses[max_samples] - SAMPLE_OFFSET;
}

unsigned int *
emu3_get_preset_addresses (struct emu3_bank *bank)
{
  char *raw = (char *) bank;

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return (unsigned int *) &raw[PRESET_SIZE_ADDR_START_EMU_3X];
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return (unsigned int *) &raw[PRESET_SIZE_ADDR_START_EMU_THREE];
  else
    return NULL;
}

int
emu3_check_bank_format (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0 ||
      strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return 1;
  else
    return 0;
}

int
emu3_get_max_samples (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return MAX_SAMPLES_EMU_3X;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return MAX_SAMPLES_EMU_THREE;
  else
    return 0;
}

int
emu3_get_max_presets (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return MAX_PRESETS_EMU_3X;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return MAX_PRESETS_EMU_THREE;

  else
    return 0;
}

void
emu3_close_file (struct emu3_file *file)
{
  free (file->raw);
  free (file);
}

struct emu3_file *
emu3_open_file (const char *filename)
{
  struct emu3_file *file;
  FILE *fd = fopen (filename, "r");

  if (fd)
    {
      file = (struct emu3_file *) malloc (sizeof (struct emu3_file));

      file->filename = filename;
      file->raw = (char *) malloc (MEM_SIZE);
      file->fsize = fread (file->raw, 1, MEM_SIZE, fd);
      fclose (fd);

      if (emu3_check_bank_format (file->bank))
	{
	  emu3_log (0, 0, "Bank name: %.*s\n", NAME_SIZE, file->bank->name);
	  emu3_log (1, 0, "Bank fsize: %zuB\n", file->fsize);
	  emu3_log (1, 0, "Bank format: %s\n", file->bank->format);

	  emu3_log (2, 1, "Geometry:\n");
	  emu3_log (2, 1, "Objects: %d\n", file->bank->objects + 1);
	  emu3_log (2, 1, "Next: 0x%08x\n", file->bank->next_sample);

	  for (int i = 0; i < BANK_PARAMETERS; i++)
	    emu3_log (2, 1, "Parameter %2d: 0x%08x (%d)\n", i,
		      file->bank->parameters[i], file->bank->parameters[i]);

	  if (file->bank->parameters[1] + file->bank->parameters[2] !=
	      file->bank->parameters[4])
	    emu3_log (2, 1, "Kind of checksum error.\n");

	  if (strncmp (file->bank->name, file->bank->name_copy, NAME_SIZE))
	    emu3_log (2, 1,
		      "Bank name is different than previously found.\n");

	  emu3_log (2, 1, "Selected preset: %d\n",
		    file->bank->selected_preset);

	  emu3_log (2, 1, "More geometry:\n");
	  for (int i = 0; i < MORE_BANK_PARAMETERS; i++)
	    emu3_log (2, 1, "Parameter %d: 0x%08x (%d)\n", i,
		      file->bank->more_parameters[i],
		      file->bank->more_parameters[i]);

	  emu3_log (2, 1, "Current preset: %d\n",
		    file->bank->more_parameters[0]);
	  emu3_log (2, 1, "Current sample: %d\n",
		    file->bank->more_parameters[1]);

	  return file;
	}
      else
	{
	  fprintf (stderr, "Bank format not supported.\n");
	  emu3_close_file (file);
	  return NULL;
	}
    }
  else
    {
      fprintf (stderr, "Error while opening %s for input\n", filename);
      return NULL;
    }
}

int
emu3_process_bank (struct emu3_file *file, int edit_preset, int xflg,
		   char *rt_controls, int level, int cutoff, int q,
		   int filter, int pbr)
{
  int size, channels;
  struct emu3_bank *bank;
  unsigned int *addresses;
  unsigned int address;
  struct emu3_preset *preset;
  struct emu3_preset_zone *zones;
  struct emu3_preset_zone_def *zone_defs;
  unsigned int sample_start_addr;
  unsigned int next_sample_addr;
  struct emu3_sample *sample;
  int max_samples;

  if (!file)
    return EXIT_FAILURE;

  addresses = emu3_get_preset_addresses (file->bank);
  for (int i = 0; i < emu3_get_max_presets (file->bank); i++)
    {
      if (addresses[0] != addresses[1]
	  && (i == edit_preset || edit_preset == -1))
	{
	  address = emu3_get_preset_address (file->bank, i);
	  preset = (struct emu3_preset *) &file->raw[address];
	  emu3_log (0, 0, "Preset %3d, %.*s", i, NAME_SIZE, preset->name);
	  emu3_log (1, 0, " @ 0x%08x", address);
	  emu3_log (0, 0, "\n");
	  if (rt_controls)
	    emu3_set_preset_rt_controls (preset, rt_controls);

	  if (pbr != -1)
	    emu3_set_preset_pbr (preset, pbr);

	  emu3_print_preset_info (preset);

	  zone_defs =
	    (struct emu3_preset_zone_def *) &file->raw[address +
						       sizeof (struct
							       emu3_preset)];
	  zones =
	    (struct emu3_preset_zone *) &file->raw[address +
						   sizeof (struct
							   emu3_preset)
						   +
						   sizeof (struct
							   emu3_preset_zone_def)
						   * preset->nzones];
	  emu3_log (1, 1, "Note mappings:\n");
	  for (int j = 0; j < NOTES; j++)
	    {
	      if (preset->note_zone_mappings[j] != 0xff)
		emu3_log (1, 2, "%s: %d\n", note_names[j],
			  preset->note_zone_mappings[j]);
	    }

	  emu3_log (1, 1, "Zones: %d\n", preset->nzones);
	  for (int j = 0; j < preset->nzones; j++)
	    {
	      emu3_log (1, 1, "Zone %d\n", j);
	      if (level != -1)
		emu3_set_preset_zone_level (zones, level);
	      if (cutoff != -1)
		emu3_set_preset_zone_cutoff (zones, cutoff);
	      if (q != -1)
		emu3_set_preset_zone_q (zones, q);
	      if (filter != -1)
		emu3_set_preset_zone_filter (zones, filter);

	      //If the zone is for pri, sec layer or both
	      if (zone_defs->type == 0xff)
		{
		  emu3_log (1, 2, "pri\n");
		  emu3_print_zone_info (zones);
		  zones++;
		}
	      else if (zone_defs->type == 0x03)
		{
		  emu3_log (1, 2, "sec\n");
		  emu3_print_zone_info (zones);
		  zones++;
		}
	      else if (zone_defs->type == 0x01)
		{
		  emu3_log (1, 2, "pri\n");
		  emu3_print_zone_info (zones);
		  zones++;
		  emu3_log (1, 2, "sec\n");
		  emu3_print_zone_info (zones);
		  zones++;
		}
	      zone_defs++;
	    }
	}
      addresses++;
    }

  sample_start_addr = emu3_get_sample_start_address (file->bank);
  emu3_log (1, 0, "Sample start: 0x%08x\n", sample_start_addr);

  max_samples = emu3_get_max_samples (file->bank);
  addresses = emu3_get_sample_addresses (file->bank);
  emu3_log (1, 0, "Start with offset: 0x%08x\n", addresses[0]);
  emu3_log (1, 0, "Next with offset: 0x%08x\n", addresses[max_samples]);
  next_sample_addr = emu3_get_next_sample_address (file->bank);
  emu3_log (1, 0, "Next sample: 0x%08x\n", next_sample_addr);

  for (int i = 0; i < max_samples; i++)
    {
      if (addresses[i] == 0)
	break;
      address = sample_start_addr + addresses[i] - SAMPLE_OFFSET;
      if (addresses[i + 1] == 0)
	size = next_sample_addr - address;
      else
	size = addresses[i + 1] - addresses[i];
      sample = (struct emu3_sample *) &file->raw[address];
      channels = emu3_get_sample_channels (sample);
      //We divide between the bytes per frame (number of channels * 2 bytes)
      //The 16 or 8 bytes are the 4 or 8 short int used for padding.
      sf_count_t nframes =
	(size - sizeof (struct emu3_sample) -
	 (8 * channels)) / (2 * channels);
      emu3_log (0, 0, "Sample %3d, %.*s", i + 1, NAME_SIZE, sample->name);
      emu3_log (1, 0, " @ 0x%08x (size %dB, frames %" PRId64 ")", address,
		size, nframes);
      emu3_log (0, 0, "\n");
      emu3_print_sample_info (sample, nframes);

      if (xflg)
	emu3_extract_sample (sample, i + 1, nframes);
    }

  return EXIT_SUCCESS;
}

int
emu3_add_sample (struct emu3_file *file, char *sample_filename, int loop)
{
  int i;
  int max_samples = emu3_get_max_samples (file->bank);
  int max_presets = emu3_get_max_presets (file->bank);
  unsigned int *saddresses = emu3_get_sample_addresses (file->bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (file->bank);
  unsigned int next_sample_addr = emu3_get_next_sample_address (file->bank);
  struct emu3_sample *sample =
    (struct emu3_sample *) &file->raw[next_sample_addr];

  if (access(sample_filename, R_OK) != 0) {
    fprintf (stderr, "Error while opening %s for input\n", sample_filename);
    return EXIT_FAILURE;
  }

  for (i = 0; i < max_samples; i++)
    if (saddresses[i] == 0)
      break;

  if (i == max_samples)
    {
      fprintf (stderr, "No more samples could be added.\n");
      return EXIT_FAILURE;
    }

  printf ("Adding sample %d...\n", i + 1);	//Sample number is 1 based
  unsigned int size = emu3_append_sample (sample_filename, sample, loop);

  if (size)
    {
      file->bank->objects++;
      file->bank->next_sample = next_sample_addr + size - sample_start_addr;
      saddresses[i] = saddresses[max_samples];
      saddresses[max_samples] = file->bank->next_sample + SAMPLE_OFFSET;
      return EXIT_SUCCESS;
    }
  else
    {
      fprintf (stderr, "Error while adding sample.\n");
      return EXIT_FAILURE;
    }
}

void
emu3_reset_envelope (struct emu3_envelope *envelope)
{
  envelope->attack = 0;
  envelope->hold = 0;
  envelope->decay = 0;
  envelope->sustain = 0x7f;
  envelope->release = 0;
}

int
emu3_add_preset_zone (struct emu3_file *file, char *zone_params)
{
  int max_samples = emu3_get_max_samples (file->bank);
  int max_presets = emu3_get_max_presets (file->bank);
  char *sample_str = strsep (&zone_params, ",");
  char *layer = strsep (&zone_params, ",");
  char *original_key = strsep (&zone_params, ",");
  char *lower_key = strsep (&zone_params, ",");
  char *higher_key = strsep (&zone_params, ",");
  char *preset_str = strsep (&zone_params, ",");
  char *endtoken;
  unsigned int sample_num;
  unsigned int preset_num;
  unsigned int *paddresses = emu3_get_preset_addresses (file->bank);
  unsigned int *saddresses = emu3_get_sample_addresses (file->bank);
  int inc_size =
    sizeof (struct emu3_preset_zone_def) + sizeof (struct emu3_preset_zone);
  int i, total;
  void *src, *dst;
  unsigned int copy_start_addr, addr, zone_addr, zone_def_addr;
  unsigned int next_sample_addr = emu3_get_next_sample_address (file->bank);
  struct emu3_preset_zone_def *zone_def;
  struct emu3_preset_zone *zone;

  for (i = 0; i < max_samples; i++)
    if (saddresses[i] == 0)
      break;
  total = i;

  sample_num = strtol (sample_str, &endtoken, 10);
  if (*endtoken != '\0' || sample_num <= 0 || sample_num > max_samples
      || sample_num > total)
    {
      fprintf (stderr, "Illegal sample number: %d\n", sample_num);
      return EXIT_FAILURE;
    }

  for (i = 0; i < max_presets; i++)
    {
      if (paddresses[0] == paddresses[1])
	break;
      paddresses++;
    }
  total = i;

  preset_num = strtol (preset_str, &endtoken, 10);
  if (*endtoken != '\0' || preset_num < 0 || preset_num > max_presets
      || preset_num > total)
    {
      fprintf (stderr, "Illegal preset number: %d\n", preset_num);
      return EXIT_FAILURE;
    }

  int original_key_int = emu3_reverse_note_search (original_key);
  if (original_key_int == -1)
    {
      fprintf (stderr, "Illegal key %s.\n", original_key);
      return EXIT_FAILURE;
    }

  int lower_key_int = emu3_reverse_note_search (lower_key);
  if (original_key_int == -1)
    {
      fprintf (stderr, "Illegal key %s.\n", lower_key);
      return EXIT_FAILURE;
    }

  int higher_key_int = emu3_reverse_note_search (higher_key);
  if (original_key_int == -1)
    {
      fprintf (stderr, "Illegal key %s.\n", higher_key);
      return EXIT_FAILURE;
    }

  int layer_int = 0;
  if (!strcmp ("pri", layer))
    layer_int = 0xff;
  if (!strcmp ("sec", layer))
    layer_int = 0x03;

  if (!layer_int)
    {
      fprintf (stderr, "Illegal layer %s.\n", layer);
      return EXIT_FAILURE;
    }

  printf
    ("Adding %s zone for sample %d on %s (%d) from %s (%d) to %s (%d) to preset %d...\n",
     layer, sample_num, original_key, original_key_int, lower_key,
     lower_key_int, higher_key, higher_key_int, preset_num);

  copy_start_addr = emu3_get_preset_address (file->bank, preset_num + 1);

  src = &file->raw[copy_start_addr];
  dst = &file->raw[copy_start_addr + inc_size];

  paddresses = emu3_get_preset_addresses (file->bank);
  for (i = preset_num + 1; i < max_presets + 1; i++)
    paddresses[i] += inc_size;

  size_t size = next_sample_addr - copy_start_addr;

  emu3_log (1, 0, "Moving next data (%d bytes)...\n", size);

  memmove (dst, src, size);

  addr = emu3_get_preset_address (file->bank, preset_num);
  struct emu3_preset *preset = (struct emu3_preset *) &file->raw[addr];

  zone_def_addr =
    addr + sizeof (struct emu3_preset) +
    sizeof (struct emu3_preset_zone_def) * (preset->nzones);
  zone_def = (struct emu3_preset_zone_def *) &file->raw[zone_def_addr];
  zone_addr =
    zone_def_addr + sizeof (struct emu3_preset_zone_def) +
    sizeof (struct emu3_preset_zone) * (preset->nzones);
  zone = (struct emu3_preset_zone *) &file->raw[zone_addr];

  src = &file->raw[zone_def_addr];
  dst = &file->raw[zone_def_addr + sizeof (struct emu3_preset_zone_def)];

  size = sizeof (struct emu3_preset_zone) * preset->nzones;

  emu3_log (1, 0, "Moving %d zones data (%d bytes)...\n", preset->nzones,
	    size);

  memmove (dst, src, size);

  for (i = lower_key_int; i <= higher_key_int; i++)
    preset->note_zone_mappings[i] = preset->nzones;

  zone_def->unknown_1 = 0;
  zone_def->unknown_2 = 0;
  zone_def->n = preset->nzones;
  zone_def->type = layer_int;

  preset->nzones++;

  //TODO: initialize struct zone
  zone->root_note = original_key_int;
  zone->sample_id_lsb = sample_num % 256;
  zone->sample_id_msb = sample_num / 256;
  zone->parameter_a = 0x1f;
  emu3_reset_envelope (&zone->vca_envelope);
  zone->parameters_a2[0] = 0x41;
  zone->parameters_a2[1] = 0x00;
  zone->lfo_variation = 0;
  zone->vcf_cutoff = 0xef;
  zone->vcf_q = 0x80;
  zone->vcf_envelope_amount = 0;
  emu3_reset_envelope (&zone->vcf_envelope);
  emu3_reset_envelope (&zone->aux_envelope);
  zone->aux_envelope_amount = 0;
  zone->aux_envelope_dest = 0;
  zone->vel_to_vca_level = 0;
  zone->vel_to_vca_attack = 0;
  zone->vel_to_vcf_cutoff = 0;
  zone->vel_to_pitch = 0;
  zone->vel_to_aux_env = 0;
  zone->vel_to_vcf_q = 0;
  zone->vel_to_vcf_attack = 0;
  zone->vel_to_sample_start = 0;
  zone->vel_to_vca_pan = 0;
  zone->lfo_to_pitch = 0;
  zone->lfo_to_vca = 0;
  zone->lfo_to_cutoff = 0;
  zone->lfo_to_pan = 0;
  zone->vca_level = 0x7f;
  zone->unknown_1 = 0;
  zone->unknown_2 = 0x40;
  zone->unknown_3 = 0;
  zone->vca_pan = 0x40;
  zone->vcf_type_lfo_shape = 0x8;
  zone->end1 = 0xff;
  zone->end2 = 0x01;

  file->bank->next_preset += inc_size;

  return EXIT_SUCCESS;
}

int
emu3_add_preset (struct emu3_file *file, char *preset_name)
{
  int i, objects;
  unsigned int copy_start_addr;
  int max_samples = emu3_get_max_samples (file->bank);
  int max_presets = emu3_get_max_presets (file->bank);
  unsigned int *paddresses = emu3_get_preset_addresses (file->bank);
  unsigned int *saddresses = emu3_get_sample_addresses (file->bank);
  unsigned int next_sample_addr = emu3_get_next_sample_address (file->bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (file->bank);
  void *src, *dst;

  for (i = 0; i < max_samples; i++)
    if (saddresses[i] == 0)
      break;

  objects = i;

  for (i = 0; i < max_presets; i++)
    {
      if (paddresses[0] == paddresses[1])
	break;
      paddresses++;
    }

  objects += i;

  printf ("Adding preset %d...\n", i);

  if (i == max_presets)
    {
      fprintf (stderr, "No more presets could be added.\n");
      return EXIT_FAILURE;
    }

  copy_start_addr = emu3_get_preset_address (file->bank, i);

  src = &file->raw[copy_start_addr];
  dst = &file->raw[copy_start_addr + sizeof (struct emu3_preset)];

  i++;
  paddresses++;

  for (; i < max_presets + 1; i++)
    {
      paddresses[0] += sizeof (struct emu3_preset);
      paddresses++;
    }

  size_t size = next_sample_addr - copy_start_addr;

  emu3_log (1, 0, "Moving %d bytes...\n", size);

  memmove (dst, src, size);

  struct emu3_preset *new_preset = (struct emu3_preset *) src;
  emu3_cpystr (new_preset->name, preset_name);
  memcpy (new_preset->rt_controls, DEFAULT_RT_CONTROLS,
	  RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE);
  memset (new_preset->unknown_0, 0, PRESET_UNKNOWN_0_SIZE);
  new_preset->pitch_bend_range = 2;
  memset (new_preset->unknown_1, 0, PRESET_UNKNOWN_1_SIZE);
  new_preset->nzones = 0;
  memset (new_preset->note_zone_mappings, 0xff, NOTES);

  file->bank->objects = objects;
  file->bank->next_preset += sizeof (struct emu3_preset);
  file->bank->selected_preset = 0;

  return EXIT_SUCCESS;
}

void
emu3_write_file (struct emu3_file *file)
{
  FILE *fd = fopen (file->filename, "w");
  unsigned int *addresses = emu3_get_preset_addresses (file->bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (file->bank);
  size_t filesize = file->bank->next_sample + sample_start_addr;

  if (fd)
    {
      fwrite (file->raw, 1, filesize, fd);
      fclose (fd);
    }
  else
    fprintf (stderr, "Error while opening file for writing.\n");
}

int
emu3_create_bank (const char *path)
{
  int rvalue;
  struct emu3_bank bank;
  char *dirc = strdup (path);
  char *basec = strdup (path);
  char *dname = dirname (dirc);
  char *bname = basename (basec);

  char *name = emu3_str_to_emu3name (bname);
  char *src_path =
    malloc (strlen (DATADIR) + strlen (PACKAGE) + strlen (EMPTY_BANK) + 3);
  int ret = sprintf (src_path, "%s/%s/%s", DATADIR, PACKAGE, EMPTY_BANK);

  if (ret < 0)
    {
      fprintf (stderr, "Error while creating src path");
      rvalue = EXIT_FAILURE;
      goto out1;
    }

  char *dst_path = malloc (strlen (dname) + strlen (name) + 2);
  ret = sprintf (dst_path, "%s/%s", dname, name);

  if (ret < 0)
    {
      fprintf (stderr, "Error while creating dst path");
      rvalue = EXIT_FAILURE;
      goto out2;
    }

  FILE *src = fopen (src_path, "rb");
  if (!src)
    {
      fprintf (stderr, "Error while opening %s for input\n", src_path);
      rvalue = EXIT_FAILURE;
      goto out3;
    }

  FILE *dst = fopen (dst_path, "w+b");
  if (!dst)
    {
      fprintf (stderr, "Error while opening %s for output\n", dst_path);
      rvalue = EXIT_FAILURE;
      goto out4;
    }

  char buf[BUFSIZ];
  size_t size;

  while (size = fread (buf, 1, BUFSIZ, src))
    fwrite (buf, 1, size, dst);

  rewind (dst);

  if (fread (&bank, sizeof (struct emu3_bank), 1, dst))
    {
      emu3_cpystr (bank.name, name);
      emu3_cpystr (bank.name_copy, name);
      rewind (dst);
      fwrite (&bank, sizeof (struct emu3_bank), 1, dst);
    }

  printf ("File created in %s\n", dst_path);

  rvalue = EXIT_SUCCESS;

  fclose (dst);
out4:
  fclose (src);
out3:
  free (dst_path);
out2:
  free (src_path);
out1:
  free (dirc);
  free (basec);
  free (name);
  return rvalue;

}
