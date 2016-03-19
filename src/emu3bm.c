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

// This returns a valid fs name (no strange chars in filename) from the emu3fs object name
char *
emu3_fs_sample_name (const char *objname)
{
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
  fname = (char *) malloc (sizeof (char) * (size + 8));
  strncpy (fname, objname, size);
  fname[size] = '\0';
  for (i = 0; i < size; i++)
    {
      if (fname[i] == '/')
	{
	  fname[i] = '?';
	}
    }
  strcat (fname, SAMPLE_EXT);

  return fname;
}

int
emu3_get_sample_channels (struct emu3_sample *sample)
{
  int channels;

  switch (sample->parameters[10])
    {
    case STEREO_SAMPLE:
      channels = 2;
      break;
    case MONO_SAMPLE:
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
emu3_print_sample_info (struct emu3_sample *sample)
{
  for (int i = 0; i < SAMPLE_PARAMETERS; i++)
    {
      printf ("0x%08x ", sample->parameters[i]);
    }
  printf ("\n");
  printf ("Channels: %d\n", emu3_get_sample_channels (sample));
  printf ("Sampling frequency: %dHz?\n", sample->parameters[9]);
}

//returns the sample size in bytes that the the sample takes in the bank
int
emu3_append_sample (const char *filename, struct emu3_sample *sample,
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

  input_info.format = 0;
  input = sf_open (filename, SFM_READ, &input_info);
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
      printf ("Appending sample %s... (%lld frames, %d channels)\n", filename,
	      input_info.frames, input_info.channels);
      //Sample header initialization
      snprintf (sample->name, NAME_SIZE, "Sample %.3d      ", sample_id);
      sample->name[NAME_SIZE - 1] = ' ';
      for (int i = 0; i < SAMPLE_PARAMETERS; i++)
	{
	  sample->parameters[i] = 0;
	}
      //TODO: complete
      data_size = sizeof (short int) * input_info.frames;
      mono_size = sizeof (struct emu3_sample) + data_size + 4;
      size = mono_size + (input_info.channels == 1 ? 0 : data_size + 8);
      sample->parameters[1] = 0x5c;
      sample->parameters[2] = input_info.channels == 1 ? 0 : mono_size;
      sample->parameters[3] = mono_size - 2;
      sample->parameters[4] = size - (input_info.channels == 1 ? 94 : 2);
      sample->parameters[5] = 0x68;
      sample->parameters[6] = (input_info.channels == 1 ? 0 : size - 20);
      sample->parameters[7] = mono_size - 12;
      sample->parameters[8] =
	sample->parameters[4] - (input_info.channels == 1 ? 0 : 10);
      sample->parameters[9] = DEFAULT_SAMPLING_FREQ;
      sample->parameters[10] =
	input_info.channels == 1 ? MONO_SAMPLE : STEREO_SAMPLE;
      sample->parameters[11] =
	address + 0x5c - (sample_id ==
			  1 ? 0 : (sample_id - 1) * 10) +
	(input_info.channels == 1 ? 0 : sample->parameters[3]);
      //sample->parameters[11] = address + 0x5c - (sample_id == 1 ? 0 : (sample_id - 1) * 10) + (input_info.channels == 1 ? 0 : sample->parameters[3] * 2);

      sample->parameters[12] =
	sample->parameters[11] + (input_info.channels == 1 ? 0 : mono_size);

      //TODO: is the following line true for mono and stereo samples?
      //total size is sample + data size + 4 \0 chars at the end
      l_channel = sample->frames;
      if (input_info.channels == 2)
	{
	  r_channel = sample->frames + input_info.frames + 2;
	  *r_channel = 0;
	  r_channel++;
	  *r_channel = 0;
	  r_channel++;
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
      *((unsigned int *) l_channel) = 0;
      if (input_info.channels == 2)
	{
	  *((unsigned int *) r_channel) = 0;
	}
      printf ("Appended %dB (0x%08x).\n", data_size, data_size);
    }
  sf_close (input);

  return size;
}

void
emu3_write_sample_file (const char *sample_name, short *data,
			sf_count_t frames, int channels, int samplerate)
{
  SF_INFO input_info;
  SNDFILE *input;
  char *filename;

  input_info.frames = frames;
  input_info.samplerate = samplerate;
  input_info.channels = channels;
  input_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  filename = emu3_fs_sample_name (sample_name);
  printf ("Extracting sample %s...\n", filename);

  input = sf_open (filename, SFM_WRITE, &input_info);
  if (sf_writef_short (input, data, frames) != frames)
    {
      fprintf (stderr, "Error: %s\n", sf_strerror (input));
    }
  sf_close (input);
}

int
emu3_process_bank (const char *ifile, int aflg, const char *afile, int xflg)
{
  char *memory;
  FILE *file;
  int size, i;
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

  if (!(!strncmp (EMULATOR_3X_DEF, bank->signature, SIGNATURE_SIZE) ||
	!strncmp (ESI_32_V3_DEF, bank->signature, SIGNATURE_SIZE)))
    {
      printf ("Bank format not supported.\n");
      return 1;
    }

  printf ("Bank format: %s\n", bank->signature);
  printf ("Bank name: %.*s\n", NAME_SIZE, bank->bank_name);

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

  if (strncmp (bank->bank_name, bank->bank_name_copy, NAME_SIZE))
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
	  name = &(memory[address]);
	  printf ("Preset %3d, %.*s: 0x%08x\n", i, NAME_SIZE, name, address);
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
      //We divide between the bytes per frame (number of channels * 2 bytes)
      sf_count_t frames =
	(size -
	 sizeof (struct emu3_sample)) / (2 *
					 emu3_get_sample_channels (sample));
      printf ("Sample %3d - %.*s @ 0x%08x (size %dB, frames %lld)\n", i + 1,
	      NAME_SIZE, sample->name, address, size, frames);
      emu3_print_sample_info (sample);

      if (xflg)
	{
	  short *sample_start = sample->frames;
	  emu3_write_sample_file (sample->name, sample_start, frames,
				  emu3_get_sample_channels (sample),
				  sample->parameters[9]);
	}
    }

  if (aflg)
    {
      sample = (struct emu3_sample *) &memory[next_sample_addr];
      size = emu3_append_sample
	(afile, sample, next_sample_addr - sample_start_addr, i + 1);
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
  char *path =
    malloc (strlen (DATADIR) + strlen (PACKAGE) + strlen (EMPTY_BANK) + 3);
  int ret = sprintf (path, "%s/%s/%s", DATADIR, PACKAGE, EMPTY_BANK);

  if (ret < 0)
    {
      fprintf (stderr, "Error while creating full path");
      return ret;
    }
  else
    {
      char buf[BUFSIZ];
      size_t size;

      FILE *src = fopen (path, "rb");
      if (!src)
	{
	  fprintf (stderr, "Error while opening %s for input\n", path);
	  return -1;
	}

      FILE *dst = fopen (ifile, "wb");
      if (!dst)
	{
	  fprintf (stderr, "Error while opening %s for output\n", path);
	  return -1;
	}

      while (size = fread (buf, 1, BUFSIZ, src))
	{
	  fwrite (buf, 1, size, dst);
	}

      fclose (src);
      fclose (dst);
    }
}
