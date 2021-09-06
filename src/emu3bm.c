/*
 *   emu3bm.c
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

#include "emu3bm.h"

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

#define LOOP 0x00010000
#define LOOP_RELEASE 0x00080000

#define MONO_SAMPLE 0x00300001
#define MONO_SAMPLE_2 0x00500000
#define STEREO_SAMPLE 0x00700001
#define STEREO_SAMPLE_2 0x00700000
#define MONO_SAMPLE_3X 0x0030fe02

#define RT_CONTROLS_SIZE 10
#define RT_CONTROLS_FS_SIZE 2
#define PRESET_UNKNOWN_0_SIZE 16
#define PRESET_UNKNOWN_1_SIZE 8

#define ESI_32_V3_DEF      "EMU SI-32 v3   "
#define EMULATOR_3X_DEF    "EMULATOR 3X    "
#define EMULATOR_THREE_DEF "EMULATOR THREE "

#define SAMPLE_EXT ".wav"

#define EMPTY_BANK_TEMPLATE "res/empty_bank_"

#define WRITE_BUFLEN 4096

struct emu3_bank
{
  char format[FORMAT_SIZE];
  char name[NAME_SIZE];
  unsigned int objects;
  unsigned int padding[3];
  unsigned int next_preset;
  unsigned int next_sample;
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

struct emu3_preset_note_zone
{
  unsigned char unknown_1;
  unsigned char unknown_2;
  unsigned char pri_zone;
  unsigned char sec_zone;
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
  unsigned char end1;		//0xff
  unsigned char end2;		//0x01
};

struct emu3_preset
{
  char name[NAME_SIZE];
  char rt_controls[RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE];
  char unknown_0[PRESET_UNKNOWN_0_SIZE];
  char pitch_bend_range;
  char unknown_1[PRESET_UNKNOWN_1_SIZE];
  char note_zones;
  unsigned char note_zone_mappings[NOTES];
};

struct emu3_sample_descriptor
{
  short int *l_channel;
  short int *r_channel;
  struct emu3_sample *sample;
};

enum emu3_error
{
  ERR_BANK_FULL = 1,
  ERR_BAD_SAMPLE_CHANS,
  ERR_CANT_OPEN_SAMPLE,
  ERR_SAMPLE_LIMIT,
  ERR_PRESET_LIMIT,
  ERR_OPEN_BANK,
  ERR_WRITE_BANK
};

static const char *EMU3_ERROR_STR[] = {
  NULL, "Bank is full", "Sample neither mono nor stereo",
  "Error while opening sample file",
  "No more samples allowed",
  "No more presetc allowed",
  "Error while opening bank file",
  "Error while writing bank file",
};

static const char DEFAULT_RT_CONTROLS[RT_CONTROLS_SIZE +
				      RT_CONTROLS_FS_SIZE] =
  { 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 8 };

static const char *RT_CONTROLS_SRC[] = {
  "Pitch Control",
  "Mod Control",
  "Pressure Control",
  "Pedal Control",
  "MIDI A Control",
  "MIDI B Control"
};

static const int RT_CONTROLS_SRC_SIZE =
  sizeof (RT_CONTROLS_SRC) / sizeof (char *);

static const char *RT_CONTROLS_DST[] = {
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

static const int RT_CONTROLS_DST_SIZE =
  sizeof (RT_CONTROLS_DST) / sizeof (char *);

static const char *RT_CONTROLS_FS_SRC[] = {
  "Footswitch 1",
  "Footswitch 2",
};

static const int RT_CONTROLS_FS_SRC_SIZE =
  sizeof (RT_CONTROLS_FS_SRC) / sizeof (char *);

static const char *RT_CONTROLS_FS_DST[] = {
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

static const int RT_CONTROLS_FS_DST_SIZE =
  sizeof (RT_CONTROLS_FS_DST) / sizeof (char *);

static const char *LFO_SHAPE[] = {
  "triangle",
  "sine",
  "sawtooth",
  "square"
};

static const char *VCF_TYPE[] = {
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

static const int VCF_TYPE_SIZE = sizeof (VCF_TYPE) / sizeof (char *);

int verbosity;

const char *
emu3_get_err (int error)
{
  return EMU3_ERROR_STR[error];
}

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

  return wname;
}

static char *
emu3_wav_filename_to_filename (const char *wav_file)
{
  char *filename = strdup (wav_file);
  char *ext = strrchr (filename, '.');
  if (strcasecmp (ext, SAMPLE_EXT) == 0)
    *ext = '\0';
  return filename;
}

static char *
emu3_str_to_emu3name (const char *src)
{
  size_t len = strlen (src);
  if (len > NAME_SIZE)
    len = NAME_SIZE;

  char *emu3name = strndup (src, len);

  char *c = emu3name;
  for (int i = 0; i < len; i++, c++)
    if (*c < 32 || *c >= 127)
      *c = '?';

  return emu3name;
}

static void
emu3_cpystr (char *dst, const char *src)
{
  int len = strlen (src);

  memcpy (dst, src, NAME_SIZE);
  memset (&dst[len], ' ', NAME_SIZE - len);
}

static int
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

static void
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
static int
emu3_get_percent_value (char value)
{
  return (int) (value * 100 / 127.0);
}

//Pan: [0, 0x80] -> [-100, +100]
static int
emu3_get_vca_pan (char vca_pan)
{
  return (int) ((vca_pan - 0x40) * 1.5625);
}

static void
emu3_print_preset_zone_info (struct emu3_file *file,
			     struct emu3_preset_zone *zone)
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
  int q = zone->vcf_q;
  if (strcmp (ESI_32_V3_DEF, file->bank->format) == 0)
    q = q - 0x80;
  q = q * 100 / 127;

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

static void
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

static void
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

static void
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

static void
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

static void
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

static void
emu3_set_preset_zone_q (struct emu3_file *file, struct emu3_preset_zone *zone,
			int q)
{
  if (q < 0 || q > 100)
    fprintf (stderr, "Value %d for Q not in range [0, 100]\n", q);
  else
    {
      emu3_log (0, 1, "Setting Q to %d...\n", q);
      zone->vcf_q = (unsigned char) (q * 127 / 100);
      if (strcmp (ESI_32_V3_DEF, file->bank->format) == 0)
	zone->vcf_q += 0x80;
    }
}

static void
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

static void
emu3_set_preset_rt_controls (struct emu3_preset *preset, char *rt_controls)
{
  char *token;
  char *endtoken;
  int i;
  int controller;

  if (rt_controls == NULL)
    return;

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

static void
emu3_set_preset_pbr (struct emu3_preset *preset, int pbr)
{
  if (pbr < 0 || pbr > 36)
    fprintf (stderr, "Value for pitch bend range %d not in range [0, 36]\n",
	     pbr);
  else
    {
      emu3_log (0, 1, "Setting pitch bend range to %d...\n", pbr);
      preset->pitch_bend_range = pbr;
    }
}

static void
emu3_init_sample_descriptor (struct emu3_sample_descriptor *sd,
			     struct emu3_sample *sample, int frames)
{
  sd->sample = sample;

  sd->l_channel = sample->frames;
  if ((sample->format & STEREO_SAMPLE) == STEREO_SAMPLE)
    //We consider the 4 shorts padding that the left channel has
    sd->r_channel = sample->frames + frames + 4;
}

static void
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
static int
emu3_append_sample (struct emu3_file *file, char *path,
		    struct emu3_sample *sample, int loop)
{
  SF_INFO sfinfo;
  SNDFILE *input;
  int size;
  unsigned int data_size;
  short int frame[2];
  short int zero[] = { 0, 0 };
  int mono_size;
  const char *filename;
  struct emu3_sample_descriptor sd;

  if (access (path, R_OK) != 0)
    return -ERR_CANT_OPEN_SAMPLE;

  size = -1;
  sfinfo.format = 0;
  input = sf_open (path, SFM_READ, &sfinfo);

  if (sfinfo.channels > 2)
    {
      size = -ERR_BAD_SAMPLE_CHANS;
      goto close;
    }

  data_size = sizeof (short int) * (sfinfo.frames + 4);
  mono_size = sizeof (struct emu3_sample) + data_size;
  size = mono_size + (sfinfo.channels == 1 ? 0 : data_size);

  if (file->fsize + size > MEM_SIZE)
    {
      size = -ERR_BANK_FULL;
      goto close;
    }

  char *basec = strdup (path);
  filename = basename (basec);
  emu3_log (1, 0,
	    "Appending sample %s... (%" PRId64
	    " frames, %d channels)\n", filename, sfinfo.frames,
	    sfinfo.channels);
  //Sample header initialization
  char *name = emu3_wav_filename_to_filename (filename);
  char *emu3name = emu3_str_to_emu3name (name);
  emu3_cpystr (sample->name, emu3name);

  free (basec);
  free (name);
  free (emu3name);

  sample->parameters[0] = 0;
  //Start of left channel
  sample->parameters[1] = sizeof (struct emu3_sample);
  //Start of right channel
  sample->parameters[2] = sfinfo.channels == 1 ? 0 : mono_size;
  //Last sample of left channel
  sample->parameters[3] = mono_size - 2;
  //Last sample of right channel
  sample->parameters[4] = sfinfo.channels == 1 ? 0 : size - 2;

  int loop_start = 4;		//This is a constant
  //Example (mono and stereo): Loop start @ 9290 sample is stored as ((9290 + 2) * 2) + sizeof (struct emu3_sample)
  sample->parameters[5] =
    ((loop_start + 2) * 2) + sizeof (struct emu3_sample);
  //Example
  //Mono: Loop start @ 9290 sample is stored as (9290 + 2) * 2
  //Stereo: Frames * 2 + parameters[5] + 8
  sample->parameters[6] =
    sfinfo.channels ==
    1 ? (loop_start + 2) * 2 : sfinfo.frames * 2 + sample->parameters[5] + 8;

  int loop_end = sfinfo.frames - 10;	//This is a constant
  //Example (mono and stereo): Loop end @ 94008 sample is stored as ((94008 + 1) * 2) + sizeof (struct emu3_sample)
  sample->parameters[7] = ((loop_end + 1) * 2) + sizeof (struct emu3_sample);
  //Example
  //Mono: Loop end @ 94008 sample is stored as ((94008 + 1) * 2)
  //Stereo: Frames * 2 + parameters[7] + 8
  sample->parameters[8] =
    sfinfo.channels ==
    1 ? (loop_end + 1) * 2 : sfinfo.frames * 2 + sample->parameters[7] + 8;

  sample->sample_rate = sfinfo.samplerate;

  sample->format = sfinfo.channels == 1 ? MONO_SAMPLE : STEREO_SAMPLE;

  if (loop)
    sample->format = sample->format | LOOP | LOOP_RELEASE;

  for (int i = 0; i < MORE_SAMPLE_PARAMETERS; i++)
    sample->more_parameters[i] = 0;

  emu3_init_sample_descriptor (&sd, sample, sfinfo.frames);

  //2 first frames set to 0
  emu3_write_frame (&sd, zero);
  emu3_write_frame (&sd, zero);

  for (int i = 0; i < sfinfo.frames; i++)
    {
      sf_readf_short (input, frame, 1);
      emu3_write_frame (&sd, frame);
    }

  //2 end frames set to 0
  emu3_write_frame (&sd, zero);
  emu3_write_frame (&sd, zero);

  emu3_log (1, 1, "Appended %dB (0x%08x).\n", size, size);

close:
  sf_close (input);

  return size;
}

static void
emu3_extract_sample (struct emu3_sample *sample, int num, sf_count_t nframes,
		     int ext_mode)
{
  SF_INFO sfinfo;
  SNDFILE *output;
  char *wav_file;
  short *l_channel, *r_channel;
  short frame[2];
  char *schannels;
  int channels = emu3_get_sample_channels (sample);

  sfinfo.frames = nframes;
  sfinfo.samplerate = sample->sample_rate;
  sfinfo.channels = channels;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  wav_file = emu3_emu3name_to_wav_filename (sample->name, num, ext_mode);
  schannels = channels == 1 ? "mono" : "stereo";
  emu3_log (0, 1, "Extracting %s sample %s...\n", schannels, wav_file);

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
	  fprintf (stderr, "Error: %s\n", sf_strerror (output));
	  break;
	}
    }

  free (wav_file);
  sf_close (output);
}

static unsigned int *
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

static unsigned int
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

static int
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

static unsigned int
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

static unsigned int *
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

static int
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

static unsigned int
emu3_get_next_sample_address (struct emu3_bank *bank)
{
  int max_samples = emu3_get_max_samples (bank);
  unsigned int *saddresses = emu3_get_sample_addresses (bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (bank);

  return sample_start_addr + saddresses[max_samples] - SAMPLE_OFFSET;
}

static int
emu3_check_bank_format (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0 ||
      strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return 1;
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

  if (!fd)
    {
      fprintf (stderr, "Error while opening %s for input\n", filename);
      return NULL;
    }

  file = (struct emu3_file *) malloc (sizeof (struct emu3_file));

  file->filename = filename;
  file->raw = (char *) malloc (MEM_SIZE);
  file->fsize = fread (file->raw, 1, MEM_SIZE, fd);
  fclose (fd);

  if (!emu3_check_bank_format (file->bank))
    {
      fprintf (stderr, "Bank format not supported.\n");
      emu3_close_file (file);
      return NULL;
    }

  emu3_log (0, 0, "Bank name: %.*s\n", NAME_SIZE, file->bank->name);
  emu3_log (1, 0, "Bank fsize: %zuB\n", file->fsize);
  emu3_log (1, 0, "Bank format: %s\n", file->bank->format);

  emu3_log (2, 1, "Geometry:\n");
  emu3_log (2, 1, "Objects: %d\n", file->bank->objects + 1);
  emu3_log (2, 1, "Next sample: 0x%08x\n", file->bank->next_sample);

  for (int i = 0; i < BANK_PARAMETERS; i++)
    emu3_log (2, 1, "Parameter %2d: 0x%08x (%d)\n", i,
	      file->bank->parameters[i], file->bank->parameters[i]);

  if (file->bank->parameters[1] + file->bank->parameters[2] !=
      file->bank->parameters[4])
    emu3_log (2, 1, "Kind of checksum error.\n");

  if (strncmp (file->bank->name, file->bank->name_copy, NAME_SIZE))
    emu3_log (2, 1, "Bank name is different than previously found.\n");

  emu3_log (2, 1, "Selected preset: %d\n", file->bank->selected_preset);

  emu3_log (2, 1, "More geometry:\n");
  for (int i = 0; i < MORE_BANK_PARAMETERS; i++)
    emu3_log (2, 1, "Parameter %d: 0x%08x (%d)\n", i,
	      file->bank->more_parameters[i], file->bank->more_parameters[i]);

  emu3_log (2, 1, "Current preset: %d\n", file->bank->more_parameters[0]);
  emu3_log (2, 1, "Current sample: %d\n", file->bank->more_parameters[1]);

  return file;
}

static void
emu3_process_zone (struct emu3_file *file, struct emu3_preset_zone *zone,
		   int level, int cutoff, int q, int filter)
{
  if (level != -1)
    emu3_set_preset_zone_level (zone, level);
  if (cutoff != -1)
    emu3_set_preset_zone_cutoff (zone, cutoff);
  if (q != -1)
    emu3_set_preset_zone_q (file, zone, q);
  if (filter != -1)
    emu3_set_preset_zone_filter (zone, filter);
}

static void
emu3_process_note_zone (struct emu3_file *file,
			struct emu3_preset_zone *zones,
			struct emu3_preset_note_zone *note_zone, int level,
			int cutoff, int q, int filter)
{
  //If the zone is for pri, sec layer or both
  if (note_zone->pri_zone != 0xff)
    {
      emu3_log (1, 2, "pri\n");
      struct emu3_preset_zone *zone = &zones[note_zone->pri_zone];
      emu3_process_zone (file, zone, level, cutoff, q, filter);
      emu3_print_preset_zone_info (file, zone);
    }
  if (note_zone->sec_zone != 0xff)
    {
      emu3_log (1, 2, "sec\n");
      struct emu3_preset_zone *zone = &zones[note_zone->sec_zone];
      emu3_process_zone (file, zone, level, cutoff, q, filter);
      emu3_print_preset_zone_info (file, zone);
    }
}

static int
emu3_process_preset (struct emu3_file *file, int preset_num,
		     char *rt_controls, int pbr, int level, int cutoff, int q,
		     int filter)
{
  unsigned int address = emu3_get_preset_address (file->bank, preset_num);
  struct emu3_preset *preset = (struct emu3_preset *) &file->raw[address];
  struct emu3_preset_zone *zones;
  struct emu3_preset_note_zone *note_zones;

  emu3_log (0, 0, "Preset %3d, %.*s", preset_num, NAME_SIZE, preset->name);
  emu3_log (1, 0, " @ 0x%08x", address);
  emu3_log (0, 0, "\n");

  if (rt_controls)
    {
      char *rtc = strdup (rt_controls);
      emu3_set_preset_rt_controls (preset, rtc);
      free (rtc);
    }

  if (pbr != -1)
    emu3_set_preset_pbr (preset, pbr);

  emu3_print_preset_info (preset);

  note_zones =
    (struct emu3_preset_note_zone *) &file->raw[address +
						sizeof (struct emu3_preset)];
  zones =
    (struct emu3_preset_zone *) &file->raw[address +
					   sizeof (struct
						   emu3_preset)
					   +
					   sizeof (struct
						   emu3_preset_note_zone)
					   * preset->note_zones];
  emu3_log (1, 1, "Note mappings:\n");
  for (int j = 0; j < NOTES; j++)
    {
      if (preset->note_zone_mappings[j] != 0xff)
	emu3_log (1, 2, "%s: %d\n", note_names[j],
		  preset->note_zone_mappings[j]);
    }

  emu3_log (1, 1, "Zones: %d\n", preset->note_zones);
  for (int j = 0; j < preset->note_zones; j++)
    {
      emu3_log (1, 1, "Zone %d\n", j);
      emu3_process_note_zone (file, zones, note_zones, level, cutoff, q,
			      filter);
      note_zones++;
    }
}

static int
emu3_get_sample_size (int next_sample_addr, unsigned int *addresses,
		      unsigned int address)
{
  int size;

  if (addresses[1] == 0)
    size = next_sample_addr - address;
  else
    size = addresses[1] - addresses[0];

  return size;
}

int
emu3_process_bank (struct emu3_file *file, int ext_mode, int edit_preset,
		   char *rt_controls, int pbr, int level, int cutoff, int q,
		   int filter)
{
  int i, size, channels;
  struct emu3_bank *bank;
  unsigned int *addresses;
  unsigned int address;
  unsigned int sample_start_addr;
  unsigned int next_sample_addr;
  struct emu3_sample *sample;
  int max_samples;
  int max_presets = emu3_get_max_presets (file->bank);

  i = 0;
  addresses = emu3_get_preset_addresses (file->bank);
  while (addresses[0] != addresses[1] && i < max_presets)
    {
      emu3_process_preset (file, i, rt_controls, pbr, level, cutoff, q,
			   filter);
      addresses++;
      i++;
    }

  sample_start_addr = emu3_get_sample_start_address (file->bank);
  emu3_log (1, 0, "Sample start: 0x%08x\n", sample_start_addr);

  max_samples = emu3_get_max_samples (file->bank);
  addresses = emu3_get_sample_addresses (file->bank);
  emu3_log (1, 0, "Start with offset: 0x%08x\n", addresses[0]);
  emu3_log (1, 0, "Next with offset: 0x%08x\n", addresses[max_samples]);
  next_sample_addr = emu3_get_next_sample_address (file->bank);
  emu3_log (1, 0, "Next sample: 0x%08x (equals bank size)\n",
	    next_sample_addr);

  i = 0;
  while (addresses[i] != 0 && i < max_samples)
    {
      address = sample_start_addr + addresses[i] - SAMPLE_OFFSET;

      size = emu3_get_sample_size (next_sample_addr, &addresses[i], address);

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

      if (ext_mode)
	emu3_extract_sample (sample, i + 1, nframes, ext_mode);

      i++;
    }

  return EXIT_SUCCESS;
}

static int
emu3_get_bank_presets (struct emu3_bank *bank)
{
  unsigned int *paddresses = emu3_get_preset_addresses (bank);
  int max_presets = emu3_get_max_presets (bank);
  int total = 0;

  while (paddresses[0] != paddresses[1] && total < max_presets)
    {
      paddresses++;
      total++;
    }

  return total;
}

static int
emu3_get_bank_samples (struct emu3_bank *bank)
{
  unsigned int *saddresses = emu3_get_sample_addresses (bank);
  int max_samples = emu3_get_max_samples (bank);
  int total = 0;

  while (saddresses[0] != 0 && total < max_samples)
    {
      saddresses++;
      total++;
    }

  return total;
}

static int
emu3_get_bank_objects (struct emu3_bank *bank)
{
  return emu3_get_bank_samples (bank) + emu3_get_bank_presets (bank);
}

int
emu3_add_sample (struct emu3_file *file, char *sample_filename, int loop)
{
  int i;
  int max_samples = emu3_get_max_samples (file->bank);
  int total_samples = emu3_get_bank_samples (file->bank);
  unsigned int *saddresses = emu3_get_sample_addresses (file->bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (file->bank);
  unsigned int next_sample_addr = emu3_get_next_sample_address (file->bank);
  struct emu3_sample *sample =
    (struct emu3_sample *) &file->raw[next_sample_addr];

  if (total_samples == max_samples)
    {
      return ERR_SAMPLE_LIMIT;
    }

  fprintf (stderr, "Adding sample %d...\n", total_samples + 1);	//Sample number is 1 based
  int size = emu3_append_sample (file, sample_filename, sample, loop);

  if (size < 0)
    {
      return -size;
    }

  file->bank->objects++;
  file->bank->next_sample = next_sample_addr + size - sample_start_addr;
  saddresses[total_samples] = saddresses[max_samples];
  saddresses[max_samples] = file->bank->next_sample + SAMPLE_OFFSET;
  file->fsize += size;

  return EXIT_SUCCESS;
}

static void
emu3_reset_envelope (struct emu3_envelope *envelope)
{
  envelope->attack = 0;
  envelope->hold = 0;
  envelope->decay = 0;
  envelope->sustain = 0x7f;
  envelope->release = 0;
}

struct emu3_preset *
emu3_get_preset (struct emu3_file *file, int preset_num)
{
  unsigned int addr = emu3_get_preset_address (file->bank, preset_num);
  return (struct emu3_preset *) &file->raw[addr];
}

static unsigned int
emu3_get_preset_note_zone_addr (struct emu3_file *file, int preset_num)
{
  unsigned int addr = emu3_get_preset_address (file->bank, preset_num);
  return addr + sizeof (struct emu3_preset);
}

static unsigned int
emu3_get_preset_zone_addr (struct emu3_file *file, int preset_num)
{
  struct emu3_preset *preset = emu3_get_preset (file, preset_num);
  return emu3_get_preset_note_zone_addr (file, preset_num) +
    (preset->note_zones) * sizeof (struct emu3_preset_note_zone);
}

struct emu3_preset_note_zone *
emu3_get_preset_note_zone (struct emu3_file *file, int preset_num)
{
  unsigned int addr = emu3_get_preset_note_zone_addr (file, preset_num);
  return (struct emu3_preset_note_zone *) &file->raw[addr];
}

static int
emu3_get_preset_zones (struct emu3_file *file, int preset_num)
{
  int i, max = -1;
  struct emu3_preset *preset = emu3_get_preset (file, preset_num);
  struct emu3_preset_note_zone *note_zone =
    emu3_get_preset_note_zone (file, preset_num);

  for (i = 0; i < preset->note_zones; i++)
    {
      if (note_zone->pri_zone != 0xff && note_zone->pri_zone > max)
	max = note_zone->pri_zone;
      if (note_zone->sec_zone != 0xff && note_zone->sec_zone > max)
	max = note_zone->sec_zone;
      note_zone++;
    }
  return max + 1;
}

static int
emu3_add_zones (struct emu3_file *file, int preset_num, int zone_num,
		struct emu3_preset_zone **preset_zone)
{
  void *src, *dst, *zone_src, *zone_dst;
  unsigned int next_preset_addr, dst_addr;
  unsigned int *paddresses;
  size_t size, inc_size;
  unsigned int next_sample_addr;
  struct emu3_preset *preset;
  struct emu3_preset_note_zone *note_zone;
  int max_presets;

  inc_size = sizeof (struct emu3_preset_zone);
  if (zone_num == -1)
    inc_size += sizeof (struct emu3_preset_note_zone);

  if (file->fsize + inc_size > MEM_SIZE)
    return -1;

  next_preset_addr = emu3_get_preset_address (file->bank, preset_num + 1);
  dst_addr = next_preset_addr + inc_size;

  paddresses = emu3_get_preset_addresses (file->bank);
  max_presets = emu3_get_max_presets (file->bank);
  for (int i = preset_num + 1; i < max_presets + 1; i++)
    paddresses[i] += inc_size;

  next_sample_addr = emu3_get_next_sample_address (file->bank);
  size = next_sample_addr - next_preset_addr;

  emu3_log (2, 0, "Moving %dB from 0x%08x to 0x%08x...\n", size,
	    next_preset_addr, dst_addr);

  src = &file->raw[next_preset_addr];
  dst = &file->raw[dst_addr];
  memmove (dst, src, size);

  preset = emu3_get_preset (file, preset_num);

  if (zone_num == -1)
    {
      unsigned int zone_addr = emu3_get_preset_zone_addr (file, preset_num);
      unsigned int zone_addr_dst =
	zone_addr + sizeof (struct emu3_preset_note_zone);

      zone_src = &file->raw[zone_addr];
      zone_dst = &file->raw[zone_addr_dst];

      if (preset->note_zones > 0)
	{
	  size = next_preset_addr - zone_addr;

	  emu3_log (2, 0, "Moving %dB from from 0x%08x to 0x%08x...\n", size,
		    zone_addr, zone_addr_dst);

	  memmove (zone_dst, zone_src, size);
	}

      note_zone = zone_src;
      note_zone->unknown_1 = 0;
      note_zone->unknown_2 = 0;
      note_zone->pri_zone = preset->note_zones;
      note_zone->sec_zone = 0xff;

      unsigned int next_zone_addr =
	next_preset_addr + sizeof (struct emu3_preset_note_zone);

      *preset_zone = (struct emu3_preset_zone *) &file->raw[next_zone_addr];

      preset->note_zones++;
    }
  else
    {
      note_zone = &emu3_get_preset_note_zone (file, preset_num)[zone_num];
      note_zone->sec_zone = emu3_get_preset_zones (file, preset_num);

      *preset_zone = (struct emu3_preset_zone *) &file->raw[next_preset_addr];
    }

  return inc_size;
}

int
emu3_add_preset_zone (struct emu3_file *file, int preset_num, int sample_num,
		      struct emu3_zone_range *zone_range)
{
  int sec_zone_id;
  int total_presets = emu3_get_bank_presets (file->bank);
  int total_samples = emu3_get_bank_samples (file->bank);
  unsigned int addr, zone_addr, zone_def_addr, inc_size;
  struct emu3_preset *preset;
  struct emu3_preset_zone *zone;

  if (preset_num < 0 || preset_num >= total_presets)
    {
      fprintf (stderr, "Illegal preset number: %d\n", preset_num);
      return EXIT_FAILURE;
    }

  emu3_log (1, 0,
	    "Adding sample %d to layer %d with original key %s (%d) from %s (%d) to %s (%d) to preset %d...\n",
	    sample_num, zone_range->layer,
	    note_names[zone_range->original_key], zone_range->original_key,
	    note_names[zone_range->lower_key], zone_range->lower_key,
	    note_names[zone_range->higher_key], zone_range->higher_key,
	    preset_num);

  preset = emu3_get_preset (file, preset_num);

  if (zone_range->layer == 1)
    {
      int assigned = 0;
      for (int i = zone_range->lower_key; i <= zone_range->higher_key; i++)
	if (preset->note_zone_mappings[i] != 0xff)
	  assigned = 1;
      if (assigned == 1)
	{
	  fprintf (stderr, "Zone already assigned to notes.\n");
	  return EXIT_FAILURE;
	}

      for (int i = zone_range->lower_key; i <= zone_range->higher_key; i++)
	preset->note_zone_mappings[i] = preset->note_zones;

      inc_size = emu3_add_zones (file, preset_num, -1, &zone);
      if (inc_size < 0)
	return ERR_BANK_FULL;
    }
  else if (zone_range->layer == 2)
    {
      sec_zone_id = preset->note_zone_mappings[zone_range->lower_key];
      int several = 0;
      for (int i = zone_range->lower_key + 1; i <= zone_range->higher_key;
	   i++)
	if (preset->note_zone_mappings[i] != sec_zone_id)
	  several = 1;
      if (several == 1)
	{
	  fprintf (stderr, "Note range in several zones.\n");
	  return EXIT_FAILURE;
	}

      inc_size = emu3_add_zones (file, preset_num, sec_zone_id, &zone);
      if (inc_size < 0)
	return ERR_BANK_FULL;
    }

  zone->root_note = zone_range->original_key;
  zone->sample_id_lsb = sample_num % 256;
  zone->sample_id_msb = sample_num / 256;
  zone->parameter_a = 0x1f;
  emu3_reset_envelope (&zone->vca_envelope);
  zone->parameters_a2[0] = 0x41;
  zone->parameters_a2[1] = 0x00;
  zone->lfo_variation = 0;
  zone->vcf_cutoff = 0xef;
  zone->vcf_q = strcmp (ESI_32_V3_DEF, file->bank->format) == 0 ? 0x80 : 0;
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
  file->fsize += inc_size;

  return EXIT_SUCCESS;
}

int
emu3_add_preset (struct emu3_file *file, char *preset_name)
{
  int i, objects;
  unsigned int copy_start_addr;
  int max_presets = emu3_get_max_presets (file->bank);
  unsigned int *paddresses = emu3_get_preset_addresses (file->bank);
  unsigned int next_sample_addr = emu3_get_next_sample_address (file->bank);
  unsigned int sample_start_addr = emu3_get_sample_start_address (file->bank);
  void *src, *dst;

  objects = emu3_get_bank_samples (file->bank);

  for (i = 0; i < max_presets; i++)
    {
      if (paddresses[0] == paddresses[1])
	break;
      paddresses++;
    }

  if (i == max_presets)
    {
      return ERR_PRESET_LIMIT;
    }

  fprintf (stderr, "Adding preset %d...\n", i);

  objects += i;

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

  if (file->fsize + size > MEM_SIZE)
    return ERR_BANK_FULL;

  emu3_log (1, 0, "Moving %dB...\n", size);

  memmove (dst, src, size);

  struct emu3_preset *new_preset = (struct emu3_preset *) src;
  emu3_cpystr (new_preset->name, preset_name);
  memcpy (new_preset->rt_controls, DEFAULT_RT_CONTROLS,
	  RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE);
  memset (new_preset->unknown_0, 0, PRESET_UNKNOWN_0_SIZE);
  new_preset->pitch_bend_range = 2;
  memset (new_preset->unknown_1, 0, PRESET_UNKNOWN_1_SIZE);
  new_preset->note_zones = 0;
  memset (new_preset->note_zone_mappings, 0xff, NOTES);

  file->bank->objects = objects;
  file->bank->next_preset += sizeof (struct emu3_preset);
  file->bank->selected_preset = 0;
  file->fsize += sizeof (struct emu3_preset);

  return EXIT_SUCCESS;
}

int
emu3_write_file (struct emu3_file *file)
{
  int err = 0;
  FILE *fd = fopen (file->filename, "w");
  if (!fd)
    {
      return ERR_OPEN_BANK;
    }

  if (fwrite (file->raw, file->fsize, 1, fd) != 1)
    {
      fprintf (stderr, "Unexpected written bytes amount.\n");
      err = ERR_WRITE_BANK;
    }

  fclose (fd);
  return err;
}

int
emu3_create_bank (const char *path, const char *type)
{
  int rvalue;
  struct emu3_bank bank;
  char *dirc = strdup (path);
  char *basec = strdup (path);
  char *dname = dirname (dirc);
  char *bname = basename (basec);

  char *name = emu3_str_to_emu3name (bname);
  char *src_path =
    malloc (strlen (DATADIR) + strlen (PACKAGE) +
	    strlen (EMPTY_BANK_TEMPLATE) + strlen (type) + 3);
  int ret =
    sprintf (src_path, "%s/%s/%s%s", DATADIR, PACKAGE, EMPTY_BANK_TEMPLATE,
	     type);

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

  fprintf (stderr, "File created in %s\n", dst_path);

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
