/*
 *	emu3bm.c
 *	Copyright (C) 2016 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   EMU3 Filesystem Tools is free software: you can redistribute it and/or modify
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

#include "emu3bm.h"

const char *RT_CONTROLS_SRC[] = {
  "Pitch Control",
  "Mod Control",
  "Pressure Control",
  "Pedal Control",
  "MIDI A Control",
  "MIDI B Control"
};

const char *RT_CONTROLS_DST[] = {
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

const char *RT_CONTROLS_FS_SRC[] = {
  "Footswitch 1",
  "Footswitch 2",
};

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

char *
  emu3_e3name_to_filename (const char *objname) {
  int i, size;
  const char *index = &objname[NAME_SIZE - 1];
  char *fname;

  for (size = NAME_SIZE; size > 0; size--)
    {
      if (*index != ' ')
	{
	  break;
	}
      index--;
    }
  fname = (char *) malloc (size + 1);
  strncpy (fname, objname, size);
  fname[size] = '\0';
  for (i = 0; i < size; i++)
    {
      if (fname[i] == '/')
	{
	  fname[i] = '?';
	}
    }

  return fname;
}

char *
emu3_e3name_to_wav_filename (const char *e3name)
{
  char *fname = emu3_e3name_to_filename (e3name);
  char *wname = malloc (strlen (fname) + 5);
  strcpy (wname, fname);
  strcat (wname, SAMPLE_EXT);
  return wname;
}

//TODO: complete add non case sensitive
char *
emu3_wav_filename_to_filename (const char *wav_file)
{
  char *filename = malloc (strlen (wav_file) + 1);
  strcpy (filename, wav_file);
  char *ext = strrchr (wav_file, '.');
  if (strcmp (ext, SAMPLE_EXT) == 0)
    {
      free (filename);
      int len_wo_ext = strlen (wav_file) - strlen (SAMPLE_EXT);
      filename = malloc (len_wo_ext + 1);
      strncpy (filename, wav_file, len_wo_ext);
      filename[len_wo_ext] = '\0';
    }
  return filename;
}

char *
emu3_str_to_e3name (const char *src)
{
  int len = strlen (src);
  char *e3name = malloc (len + 1);
  strcpy (e3name, src);
  char *c = e3name;
  for (int i = 0; i < len; i++, c++)
    {
      if (!isalnum (*c) && *c != ' ' && *c != '#')
	{
	  *c = '?';
	}
    }
  return e3name;
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
  int channels;

  switch (sample->parameters[10])
    {
    case STEREO_SAMPLE_1:
    case STEREO_SAMPLE_2:
      channels = 2;
      break;
    case MONO_SAMPLE_1:
    case MONO_SAMPLE_2:
    case MONO_SAMPLE_EMULATOR_3X_1:
    case MONO_SAMPLE_EMULATOR_3X_2:
    case MONO_SAMPLE_EMULATOR_3X_3:
    case MONO_SAMPLE_EMULATOR_3X_4:
    case MONO_SAMPLE_EMULATOR_3X_5:
    default:
      channels = 1;
      break;
    }
  return channels;
}

void
emu3_print_sample_info (struct emu3_sample *sample, sf_count_t nframes)
{
  for (int i = 0; i < SAMPLE_PARAMETERS; i++)
    {
      printf ("0x%08x ", sample->parameters[i]);
    }
  printf ("\n");
  printf ("Channels: %d\n", emu3_get_sample_channels (sample));
  printf ("Frames: %lld\n", nframes);
  printf ("Sampling frequency: %dHz\n", sample->parameters[9]);
}

void
emu3_print_preset_info (struct emu3_preset *preset)
{
  for (int i = 0; i < RT_CONTROLS_SIZE; i++)
    {
      if (preset->rt_controls[i])
	{
	  printf ("Mapping: %s - %s\n",
		  RT_CONTROLS_SRC[preset->rt_controls[i] - 1],
		  RT_CONTROLS_DST[i]);
	}
    }
  for (int i = 0; i < RT_CONTROLS_FS_SIZE; i++)
    {
      printf ("Mapping: %s - %s\n",
	      RT_CONTROLS_FS_SRC[i], RT_CONTROLS_FS_DST[preset->rt_controls[RT_CONTROLS_SIZE + i]]);
    }
}

//returns the sample size in bytes that the the sample takes in the bank
int
emu3_append_sample (char *path, struct emu3_sample *sample,
		    unsigned int address, int sample_id)
{
  SF_INFO input_info;
  SNDFILE *input;
  int size = 0;
  unsigned int data_size;
  short int frame[2];
  short int *l_channel;
  short int *r_channel;
  int mono_size;
  const char *filename;

  input_info.format = 0;
  input = sf_open (path, SFM_READ, &input_info);
  //TODO: add more formats
  if ((input_info.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
    {
      printf ("Sample not in a valid format. Skipping...\n");
    }
  else if (input_info.channels > 2)
    {
      printf ("Sample neither mono nor stereo. Skipping...\n");
    }
  else
    {
      filename = basename (path);
      printf ("Appending sample %s... (%lld frames, %d channels)\n", filename,
	      input_info.frames, input_info.channels);
      //Sample header initialization
      char *name = emu3_wav_filename_to_filename (filename);
      char *e3name = emu3_str_to_e3name (name);
      emu3_cpystr (sample->name, e3name);
      free (name);
      free (e3name);
      for (int i = 0; i < SAMPLE_PARAMETERS; i++)
	{
	  sample->parameters[i] = 0;
	}

      //TODO: complete
      data_size = sizeof (short int) * (input_info.frames + 4);
      mono_size = sizeof (struct emu3_sample) + data_size;
      size = mono_size + (input_info.channels == 1 ? 0 : data_size);
      sample->parameters[1] = 0x5c;
      sample->parameters[2] = input_info.channels == 1 ? 0 : mono_size;
      sample->parameters[3] = mono_size - 2;
      sample->parameters[4] = size - ((input_info.channels == 1 ? sizeof (struct emu3_sample) : 0) + 2);

      int loop_start = 4; //This is an arbitrary value
      //Example (mono and stereo): Loop start @ 9290 sample is stored as ((9290 + 2) * 2) + sizeof (struct emu3_sample)
      sample->parameters[5] = ((loop_start + 2) * 2) + sizeof (struct emu3_sample);
      //Example
      //Mono: Loop start @ 9290 sample is stored as (9290 + 2) * 2
      //Stereo: Frames * 2 + parameters[5] + 8
      sample->parameters[6] = input_info.channels == 1 ? (loop_start + 2) * 2 : input_info.frames * 2 + sample->parameters[5] + 8;

      int loop_end = input_info.frames - 10; //This is an arbitrary value
      //Example (mono and stereo): Loop end @ 94008 sample is stored as ((94008 + 1) * 2) + sizeof (struct emu3_sample)
      sample->parameters[7] = ((loop_end + 1) * 2) + sizeof (struct emu3_sample);
      //Example
      //Mono: Loop end @ 94008 sample is stored as ((94008 + 1) * 2)
      //Stereo: Frames * 2 + parameters[7] + 8
      sample->parameters[8] = input_info.channels == 1 ? (loop_end + 1) * 2: input_info.frames * 2 + sample->parameters[7] + 8;

      sample->parameters[9] = DEFAULT_SAMPLING_FREQ;

      sample->parameters[10] =
	input_info.channels == 1 ? MONO_SAMPLE_1 : STEREO_SAMPLE_1;

      sample->parameters[11] =
	address + 0x5c - (sample_id ==
			  1 ? 0 : (sample_id - 1) * 10) +
	(input_info.channels == 1 ? 0 : sample->parameters[3]);
      sample->parameters[12] =
	sample->parameters[11] + (input_info.channels == 1 ? 0 : mono_size);

      l_channel = sample->frames;
      //2 first frames set to 0
      *((unsigned int *) l_channel) = 0;
      l_channel += 2;
      if (input_info.channels == 2)
	{
	  //We consider the 4 shorts padding that the left channel has
	  r_channel = sample->frames + input_info.frames + 4;
	  *((unsigned int *) r_channel) = 0;
	  r_channel += 2;
	}
      for (int i = 0; i < input_info.frames; i++)
	{
	  sf_readf_short (input, frame, 1);
	  *l_channel = frame[0];
	  l_channel++;
	  if (input_info.channels == 2)
	    {
	      *r_channel = frame[1];
	      r_channel++;
	    }
	}
      //2 end frames set to 0
      *((unsigned int *) l_channel) = 0;
      if (input_info.channels == 2)
	{
	  *((unsigned int *) r_channel) = 0;
	}
      printf ("Appended %dB (0x%08x).\n", size, size);
    }
  sf_close (input);

  return size;
}

void
emu3_write_sample_file (struct emu3_sample *sample, sf_count_t nframes)
{
  SF_INFO output_info;
  SNDFILE *output;
  char *wav_file;
  short *l_channel, *r_channel;
  short frame[2];
  char *schannels;
  int channels = emu3_get_sample_channels (sample);
  int samplerate = sample->parameters[9];

  output_info.frames = nframes;
  output_info.samplerate = samplerate;
  output_info.channels = channels;
  output_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  wav_file = emu3_e3name_to_wav_filename (sample->name);
  schannels = channels == 1 ? "mono" : "stereo";
  printf ("Extracting %s sample %s...\n", schannels, wav_file);

  output = sf_open (wav_file, SFM_WRITE, &output_info);
  l_channel = sample->frames + 2;
  if (channels == 2)
    {
      r_channel = sample->frames + nframes + 6;
    }
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

int
emu3_process_bank (const char *ifile, int aflg, char *afile, int xflg)
{
  char *memory;
  FILE *file;
  int size, i, channels;
  size_t fsize;
  struct emu3_bank *bank;
  char *name;
  unsigned int *addresses;
  unsigned int sample_start_addr;
  unsigned int next_sample_addr;
  unsigned int address;
  struct emu3_sample *sample;

  file = fopen (ifile, "r");

  if (file == NULL)
    {
      printf ("Error: file %s could not be opened.\n", ifile);
      return -1;
    }
  memory = (char *) malloc (MEM_SIZE);

  fsize = fread (memory, 1, MEM_SIZE, file);
  fclose (file);

  printf ("Bank fsize: %dB\n", fsize);

  bank = (struct emu3_bank *) memory;

  if (strncmp (EMULATOR_3X_DEF, bank->signature, SIGNATURE_SIZE) &&
      strncmp (ESI_32_V3_DEF, bank->signature, SIGNATURE_SIZE))
    {
      printf ("Bank format not supported.\n");
      return 1;
    }

  printf ("Bank format: %s\n", bank->signature);
  printf ("Bank name: %.*s\n", NAME_SIZE, bank->name);

  printf ("Geometry:\n");
/*	for (i = 0; i < BANK_PARAMETERS; i++) {
    printf("Parameter %2d: 0x%08x (%d)\n", i, bank->parameters[i], bank->parameters[i]);
   }*/
  printf ("Objects (p 0): %d\n", bank->parameters[0] + 1);
  printf ("Unknown (p 3): 0x%08x\n", bank->parameters[3]);
  printf ("Unknown (p 4): 0x%08x\n", bank->parameters[4]);
  printf ("Next    (p 5): 0x%08x\n", bank->parameters[5]);
  printf ("Unknown (p 7): 0x%08x\n", bank->parameters[7]);
  printf ("Unknown (p 8): 0x%08x\n", bank->parameters[8]);
  printf ("Unknown (p10): 0x%08x\n", bank->parameters[10]);

  if (bank->parameters[7] + bank->parameters[8] != bank->parameters[10])
    {
      fprintf (stderr, "Kind of checksum error.\n");
    }

  if (strncmp (bank->name, bank->name_copy, NAME_SIZE))
    {
      printf ("Bank name is different than previously found.\n");
    }

  printf ("More geometry:\n");
//      for (i = 0; i < MORE_BANK_PARAMETERS; i++) {
//              printf("Parameter %d: 0x%08x (%d)\n", i, bank->more_parameters[i], bank->more_parameters[i]);
//      }
  printf ("Current preset: %d\n", bank->more_parameters[0]);
  printf ("Current sample: %d\n", bank->more_parameters[1]);

  addresses = (unsigned int *) &(memory[PRESET_SIZE_ADDR_START_EMU_3X]);
  for (i = 0; i < MAX_PRESETS; i++)
    {
      if (addresses[0] != addresses[1])
	{
	  address = PRESET_START_EMU_3X + addresses[0];
	  struct emu3_preset *preset =
	    (struct emu3_preset *) &memory[address];
	  printf ("Preset %3d, %.*s: 0x%08x\n", i, NAME_SIZE, preset->name,
		  address);
	  emu3_print_preset_info (preset);
	}
      addresses++;
    }

  sample_start_addr = PRESET_START_EMU_3X + addresses[0] + 1;	//There is always a 0xee byte before the samples
  printf ("Sample start: 0x%08x\n", sample_start_addr);

  addresses = (unsigned int *) &(memory[SAMPLE_ADDR_START_EMU_3X]);
  printf ("Start with offset: 0x%08x\n", addresses[0]);
  printf ("Next with offset: 0x%08x\n", addresses[MAX_SAMPLES - 1]);
  next_sample_addr =
    sample_start_addr + addresses[MAX_SAMPLES - 1] - SAMPLE_OFFSET;
  printf ("Next sample: 0x%08x\n", next_sample_addr);

  for (i = 0; i < MAX_SAMPLES; i++)
    {
      if (addresses[i] == 0x0)
	{
	  break;
	}
      address = sample_start_addr + addresses[i] - SAMPLE_OFFSET;
      if (addresses[i + 1] == 0x0)
	{
	  size = next_sample_addr - address;
	}
      else
	{
	  size = addresses[i + 1] - addresses[i];
	}
      sample = (struct emu3_sample *) &memory[address];
      channels = emu3_get_sample_channels (sample);
      //We divide between the bytes per frame (number of channels * 2 bytes)
      //The 16 or 8 bytes are the 4 or 8 short int used for padding.
      sf_count_t nframes =
	(size - sizeof (struct emu3_sample) -
	 (8 * channels)) / (2 * channels);
      printf ("Sample %3d - %.*s @ 0x%08x (size %dB, frames %lld)\n", i + 1,
	      NAME_SIZE, sample->name, address, size, nframes);
      emu3_print_sample_info (sample, nframes);

      if (xflg)
	{
	  emu3_write_sample_file (sample, nframes);
	}
    }

  if (aflg)
    {
      sample = (struct emu3_sample *) &memory[next_sample_addr];
      size =
	emu3_append_sample (afile, sample,
			    next_sample_addr - sample_start_addr, i + 1);
      if (size)
	{
	  addresses[i] = addresses[MAX_SAMPLES - 1];
	  addresses[MAX_SAMPLES - 1] =
	    next_sample_addr + size - sample_start_addr + SAMPLE_OFFSET;
	  bank->parameters[0]++;
	  bank->parameters[5] = next_sample_addr + size - sample_start_addr;

	  file = fopen (ifile, "w");
	  fwrite (memory, 1, next_sample_addr + size, file);
	  fclose (file);
	}
      else
	{
	  fprintf (stderr, "Error appending file.\n");
	}
    }

  return 0;
}

int
emu3_create_bank (const char *ifile)
{
  struct emu3_bank bank;
  char *name = emu3_str_to_e3name (ifile);
  char *fname = emu3_e3name_to_filename (name);
  char *path =
    malloc (strlen (DATADIR) + strlen (PACKAGE) + strlen (EMPTY_BANK) + 3);
  int ret = sprintf (path, "%s/%s/%s", DATADIR, PACKAGE, EMPTY_BANK);

  if (ret < 0)
    {
      fprintf (stderr, "Error while creating full path");
      return ret;
    }

  char buf[BUFSIZ];
  size_t size;

  FILE *src = fopen (path, "rb");
  if (!src)
    {
      fprintf (stderr, "Error while opening %s for input\n", path);
      return -1;
    }

  FILE *dst = fopen (fname, "w+b");
  if (!dst)
    {
      fprintf (stderr, "Error while opening %s for output\n", fname);
      return -1;
    }

  while (size = fread (buf, 1, BUFSIZ, src))
    {
      fwrite (buf, 1, size, dst);
    }

  fclose (src);

  rewind (dst);

  if (fread (&bank, sizeof (struct emu3_bank), 1, dst))
    {
      emu3_cpystr (bank.name, name);
      emu3_cpystr (bank.name_copy, name);
      rewind (dst);
      fwrite (&bank, sizeof (struct emu3_bank), 1, dst);
    }

  fclose (dst);

  free (name);
  free (fname);
  free (path);
}
