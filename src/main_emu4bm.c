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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
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

#define EMU4_NAME_OFFSET 2
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

static int
emu4_chunk_check_name (struct emu4_chunk *chunk, const char *name)
{
  return strncmp (chunk->name, name, CHUNK_NAME_LEN);
}

static uint32_t
emu4_chunk_get_size (struct emu4_chunk *chunk)
{
  return be32toh (chunk->size);
}

static void
emu4_chunk_print (struct emu4_chunk *chunk, char *content_name)
{
  emu_print (1, 0, "chunk %.4s: %.*s%s(%ld B)\n", chunk->name,
	     content_name == NULL ? 0 : NAME_SIZE, content_name,
	     content_name == NULL ? "" : " ", emu4_chunk_get_size (chunk));
}

static void
emu4_chunk_print_named (struct emu4_chunk *chunk)
{
  emu4_chunk_print (chunk, &chunk->data[EMU4_NAME_OFFSET]);
}

static int
emu4_chunk_add (struct emu_file *file, struct emu4_chunk *chunk)
{
  uint32_t csize = sizeof (struct emu4_chunk) + chunk->size;
  uint32_t nsize = file->size + csize;
  if (nsize >= MEM_SIZE)
    {
      return -1;
    }
  file->size = nsize;
  memcpy (&file->raw[file->size], chunk, nsize);
}

static struct emu_file *
emu4_new_file (const char *name)
{
  struct emu_file *file = emu_init_file (name);
  struct emu4_chunk *chunk = (struct emu4_chunk *) file->raw;

  memcpy (chunk->name, EMU4_FORM_TAG, CHUNK_NAME_LEN);
  chunk->size = htobe32 (strlen (EMU4_E4_FORMAT));
  memcpy (chunk->data, EMU4_E4_FORMAT, strlen (EMU4_E4_FORMAT));

  file->size = sizeof (struct emu4_chunk) + strlen (EMU4_E4_FORMAT);

  return file;
}

static int
emu4_add_sample (struct emu_file *file, struct emu4_chunk *next_chunk,
		 const char *sample_name, int force_loop)
{
  int size;
  struct emu3_sample *sample;

  next_chunk->data[0] = 0;
  next_chunk->data[1] = 0;
  sample = (struct emu3_sample *) &next_chunk->data[EMU4_NAME_OFFSET];
  size = emu3_append_sample (file, sample, sample_name, force_loop);

  if (size >= 0)
    {
      memcpy (next_chunk->name, EMU4_E3S1_TAG, CHUNK_NAME_LEN);
      next_chunk->size = htobe32 (EMU4_NAME_OFFSET + size);

      file->size += sizeof (struct emu4_chunk) + EMU4_NAME_OFFSET + size;
    }

  return size;
}

static int
emu4_process_file (struct emu_file *file, int ext_mode,
		   struct emu4_chunk **next_chunk)
{
  char *fdata;
  int err, sample_index;
  uint32_t size, total_size, chunk_size, new_size;
  struct emu4_chunk *chunk;
  struct emu3_sample *sample;
  uint32_t sample_start, sample_len;

  err = EXIT_FAILURE;
  if (next_chunk)
    {
      *next_chunk = NULL;
    }

  chunk = (struct emu4_chunk *) file->raw;
  if (!CHUNK_NAME_IS (chunk, EMU4_FORM_TAG))
    {
      emu_error ("Unexpected file type");
      goto cleanup;
    }

  emu4_chunk_print (chunk, NULL);
  total_size = emu4_chunk_get_size (chunk);

  if (strncmp (chunk->data, EMU4_E4_FORMAT, strlen (EMU4_E4_FORMAT)))
    {
      emu_error ("Unexpected format");
      goto cleanup;
    }

  chunk = (struct emu4_chunk *) &chunk->data[strlen (EMU4_E4_FORMAT)];	//EB40
  chunk_size = emu4_chunk_get_size (chunk);
  size = strlen (EMU4_E4_FORMAT);

  sample_index = 1;
  while (1)
    {
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
	  sample = (struct emu3_sample *) &chunk->data[EMU4_NAME_OFFSET];
	  emu3_process_sample (sample, sample_index, ext_mode, 0, 0);
	  sample_index++;
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
	  err = EXIT_SUCCESS;
	  if (next_chunk)
	    {
	      *next_chunk = chunk;
	    }
	  break;
	}

      if (sample_index == EMU4_MAX_SAMPLES)
	{
	  err = EXIT_SUCCESS;
	  if (next_chunk)
	    {
	      *next_chunk = NULL;
	    }
	  break;
	}

      size += sizeof (struct emu4_chunk) + chunk_size;

      chunk = (struct emu4_chunk *) &chunk->data[chunk_size];
      chunk_size = emu4_chunk_get_size (chunk);

      if (chunk_size == 0)
	{
	  break;
	}
    }

cleanup:
  free (fdata);
  return 0;
}

int
main (int argc, char *argv[])
{
  int opt;
  int nflg = 0, sflg = 0, xflg = 0, errflg = 0, totalflg;
  int ext_mode = 0;
  char *sample_name;
  int force_loop;
  int long_index = 0;
  int err = EXIT_SUCCESS;
  struct emu4_chunk *next_chunk;
  const char *bank_name;
  struct emu_file *file;

  while ((opt =
	  getopt_long (argc, argv, "hns:S:vxX", options, &long_index)) != -1)
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
	  force_loop = 0;
	  break;
	case 'S':
	  sflg++;
	  sample_name = optarg;
	  force_loop = 1;
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

  if (emu4_process_file (file, ext_mode, &next_chunk))
    {
      err = EXIT_FAILURE;
    }

  if (sflg)
    {
      err = emu4_add_sample (file, next_chunk, sample_name, force_loop);
      emu_write_file (file);
    }

  emu_close_file (file);

  exit (err);
}
