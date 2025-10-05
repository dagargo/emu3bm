/*
 *   main_emu4bm.c
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

#include <endian.h>
#include <sys/types.h>
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../config.h"
#include "emu3bm.h"
#include "sample.h"
#include "utils.h"

#define EMU4BM_PACKAGE_STRING ("emu4bm " PACKAGE_VERSION)

#define CHUNK_NAME_LEN 4

#define EMU4_FORM_TAG "FORM"
#define EMU4_TOC1_TAG "TOC1"
#define EMU4_E4MA_TAG "E4Ma"
#define EMU4_E3S1_TAG "E3S1"
#define EMU4_E4P1_TAG "E4P1"
#define EMU4_E4S1_TAG "E4s1"
#define EMU4_EMS0_TAG "EMS0"

#define EMU4_E4_FORMAT "E4B0"

#define EMU4_E3S1_OFFSET 2
#define EMU4_MAX_SAMPLES 1000

#define EMU

#define CHUNK_NAME_IS(chunk,n) (strncmp((chunk)->name, n, CHUNK_NAME_LEN) == 0)

struct emu4_chunk
{
  char name[CHUNK_NAME_LEN];
  uint32_t size;
  char data[];
};

static const struct option options[] = {
  {"new-bank", 1, NULL, 'n'},
  {"add-sample", 1, NULL, 's'},
  {"add-sample-loop", 1, NULL, 'S'},
  {"extract-samples", 0, NULL, 'x'},
  {"extract-samples-with-num", 0, NULL, 'X'},
  {"verbosity", 0, NULL, 'v'},
  {"help", 0, NULL, 'h'},
  {NULL, 0, NULL, 0}
};

static void
emu4_chunk_set_name (struct emu4_chunk *chunk, const char *name)
{
  memcpy (chunk->name, name, CHUNK_NAME_LEN);
}

static void
emu4_chunk_set_size (struct emu4_chunk *chunk, uint32_t size)
{
  chunk->size = htobe32 (size);
}

static uint32_t
emu4_chunk_get_size (struct emu4_chunk *chunk)
{
  return be32toh (chunk->size);
}

static void
emu4_chunk_print (struct emu4_chunk *chunk, char *content_name)
{
  if (content_name)
    {
      emu_print (1, 0, "chunk %.4s: %.*s (%u B)\n", chunk->name,
		 NAME_SIZE, content_name, emu4_chunk_get_size (chunk));
    }
  else
    {
      emu_print (1, 0, "chunk %.4s: (%u B)\n", chunk->name,
		 emu4_chunk_get_size (chunk));
    }
}

static void
emu4_chunk_print_named (struct emu4_chunk *chunk)
{
  emu4_chunk_print (chunk, &chunk->data[EMU4_E3S1_OFFSET]);
}

static struct emu_file *
emu4_new_file (const char *name)
{
  struct emu_file *file = emu_init_file (name);
  struct emu4_chunk *chunk = (struct emu4_chunk *) file->raw;

  emu4_chunk_set_name (chunk, EMU4_FORM_TAG);
  emu4_chunk_set_size (chunk, 0);
  memcpy (chunk->data, EMU4_E4_FORMAT, strlen (EMU4_E4_FORMAT));

  file->size = sizeof (struct emu4_chunk) + strlen (EMU4_E4_FORMAT);

  return file;
}

static int
emu4_add_sample (struct emu_file *file, struct emu4_chunk *next_chunk,
		 const char *sample_name)
{
  int size;
  uint32_t chunk_size;
  struct emu4_chunk *form_chunk;
  struct emu3_sample *sample;

  emu4_chunk_set_name (next_chunk, EMU4_E3S1_TAG);
  next_chunk->data[0] = 0;
  next_chunk->data[1] = 0;

  sample = (struct emu3_sample *) &next_chunk->data[EMU4_E3S1_OFFSET];
  size = emu3_append_sample (file, sample, sample_name);
  if (size < 0)
    {
      return 1;
    }

  emu4_chunk_set_size (next_chunk, EMU4_E3S1_OFFSET + size);
  chunk_size = sizeof (struct emu4_chunk) + EMU4_E3S1_OFFSET + size;

  file->size += chunk_size;

  form_chunk = (struct emu4_chunk *) file->raw;
  emu4_chunk_set_size (form_chunk,
		       emu4_chunk_get_size (form_chunk) + chunk_size);

  return 0;
}

static int
emu4_process_file (struct emu_file *file, int ext_mode,
		   struct emu4_chunk **next_chunk, int *sample_index)
{
  uint32_t size, total_size, chunk_size;
  struct emu4_chunk *chunk;
  struct emu3_sample *sample;

  chunk = (struct emu4_chunk *) file->raw;
  if (!CHUNK_NAME_IS (chunk, EMU4_FORM_TAG))
    {
      emu_error ("Unexpected format");
      return EXIT_FAILURE;
    }

  emu4_chunk_print (chunk, NULL);
  total_size = emu4_chunk_get_size (chunk);

  if (strncmp (chunk->data, EMU4_E4_FORMAT, strlen (EMU4_E4_FORMAT)))
    {
      emu_error ("Unexpected format");
      return EXIT_FAILURE;
    }

  chunk = (struct emu4_chunk *) &chunk->data[strlen (EMU4_E4_FORMAT)];	//EB40
  size = 0;

  *sample_index = 1;
  while (1)
    {
      if (size == total_size)
	{
	  if (next_chunk)
	    {
	      *next_chunk = chunk;
	    }
	  break;
	}

      chunk_size = emu4_chunk_get_size (chunk);
      if (chunk_size == 0)
	{
	  if (next_chunk)
	    {
	      *next_chunk = chunk;
	    }
	  break;
	}

      if (CHUNK_NAME_IS (chunk, EMU4_TOC1_TAG))
	{
	  emu4_chunk_print (chunk, NULL);
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_E4MA_TAG))
	{
	  emu4_chunk_print (chunk, NULL);
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_E3S1_TAG))
	{
	  emu4_chunk_print_named (chunk);
	  sample = (struct emu3_sample *) &chunk->data[EMU4_E3S1_OFFSET];
	  emu3_process_sample (sample, *sample_index, ext_mode, 0, 0);
	  (*sample_index)++;
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_E4P1_TAG))
	{
	  emu4_chunk_print_named (chunk);
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_E4S1_TAG))
	{
	  emu4_chunk_print_named (chunk);
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_EMS0_TAG))
	{
	  emu4_chunk_print (chunk, NULL);
	}
      else
	{
	  //The file might have more bytes than the actual used space in the bank.
	  if (next_chunk)
	    {
	      *next_chunk = chunk;
	    }
	  break;
	}

      size += sizeof (struct emu4_chunk) + chunk_size;

      chunk = (struct emu4_chunk *) &chunk->data[chunk_size];
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  int opt;
  int nflg = 0, sflg = 0, xflg = 0, errflg = 0, totalflg;
  int ext_mode = 0;
  char *sample_name = NULL;
  int long_index = 0;
  int sample_index;
  int err = EXIT_SUCCESS;
  struct emu4_chunk *next_chunk;
  const char *bank_name = NULL;
  struct emu_file *file;

  while ((opt =
	  getopt_long (argc, argv, "hns:vxX", options, &long_index)) != -1)
    {
      switch (opt)
	{
	case 'h':
	  emu_print_help (argv[0], EMU4BM_PACKAGE_STRING, options);
	  exit (EXIT_SUCCESS);
	case 'n':
	  nflg++;
	  break;
	case 's':
	  sflg++;
	  sample_name = optarg;
	  break;
	case 'v':
	  verbosity++;
	  break;
	case 'x':
	  xflg++;
	  ext_mode = EMU3_EXT_MODE_NAME;
	  break;
	case 'X':
	  xflg++;
	  ext_mode = EMU3_EXT_MODE_NAME_NUMBER;
	  break;
	case '?':
	  errflg++;
	}
    }

  if (optind + 1 == argc)
    bank_name = argv[optind];
  else
    errflg++;

  if (nflg > 1)
    errflg++;

  if (xflg > 1)
    errflg++;

  totalflg = nflg + sflg + xflg;

  if (totalflg > 1)
    errflg++;

  if (errflg > 0)
    {
      emu_print_help (argv[0], EMU4BM_PACKAGE_STRING, options);
      exit (EXIT_FAILURE);
    }

  if (nflg)
    {
      file = emu4_new_file (bank_name);
      emu_write_file (file);
    }

  file = emu_open_file (bank_name);
  if (!file)
    {
      exit (EXIT_FAILURE);
    }

  if (emu4_process_file (file, ext_mode, &next_chunk, &sample_index))
    {
      err = EXIT_FAILURE;
      goto end;
    }

  if (sflg)
    {
      if (next_chunk && sample_index < EMU4_MAX_SAMPLES)
	{
	  err = emu4_add_sample (file, next_chunk, sample_name);
	  if (!err)
	    {
	      emu_write_file (file);
	    }
	}
      else
	{
	  emu_error ("No space for more samples");
	  err = EXIT_FAILURE;
	}
    }

end:
  emu_close_file (file);

  exit (err);
}
