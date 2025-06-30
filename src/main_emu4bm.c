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
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdint.h>
#include "../config.h"
#include "sample.h"
#include "utils.h"

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
  {"extract-samples", 0, NULL, 'x'},
  {"extract-samples-with-num", 0, NULL, 'X'},
  {"verbosity", 0, NULL, 'v'},
  {"help", 0, NULL, 'h'},
  {NULL, 0, NULL, 0}
};

static void
print_help (char *executable_path)
{
  char *exec_name;
  const struct option *option;

  fprintf (stderr, "%s\n", PACKAGE_STRING);
  exec_name = basename (executable_path);
  fprintf (stderr, "Usage: %s [options] e4bank\n", exec_name);
  fprintf (stderr, "Options:\n");
  option = options;
  while (option->name)
    {
      fprintf (stderr, "  --%s, -%c", option->name, option->val);
      if (option->has_arg)
	{
	  fprintf (stderr, " value");
	}
      fprintf (stderr, "\n");
      option++;
    }
}

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

struct emu_file *
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
emu4_process_file (struct emu_file *file, int ext_mode)
{
  char *fdata;
  uint32_t size, total_size, chunk_size, new_size;
  struct emu4_chunk *chunk;
  struct emu3_sample *sample;
  int sample_index;
  uint32_t sample_start, sample_len;

  chunk = (struct emu4_chunk *) file->raw;
  if (!CHUNK_NAME_IS (chunk, EMU4_FORM_TAG))
    {
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
	  break;
	}

      if (sample_index == EMU4_MAX_SAMPLES)
	{
	  break;
	}

      new_size = size + sizeof (struct emu4_chunk) + chunk_size;
      if (new_size < size)
	{			//overflow
	  break;
	}
      size = new_size;

      if (size >= total_size || size >= file->size)
	{
	  break;
	}

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
  int nflg = 0, xflg = 0, errflg = 0, totalflg;
  int ext_mode = 0;
  int long_index = 0;
  int err = EXIT_SUCCESS;
  const char *bank_name;

  while ((opt =
	  getopt_long (argc, argv, "hnvxX", options, &long_index)) != -1)
    {
      switch (opt)
	{
	case 'h':
	  print_help (argv[0]);
	  exit (EXIT_SUCCESS);
	case 'n':
	  nflg++;
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

  totalflg = nflg + xflg;

  if (totalflg > 1)
    errflg++;

  if (errflg > 0)
    {
      print_help (argv[0]);
      exit (EXIT_FAILURE);
    }

  if (nflg)
    {
      struct emu_file *file;

      file = emu4_new_file (bank_name);

      emu_write_file (file);
      emu_close_file (file);
    }
  else
    {
      struct emu_file *file = emu_open_file (bank_name);
      if (file)
	{
	  if (emu4_process_file (file, ext_mode))
	    {
	      err = EXIT_FAILURE;
	    }
	}
      emu_close_file (file);
    }

  exit (err);
}
