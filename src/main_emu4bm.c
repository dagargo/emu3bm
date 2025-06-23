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

#define EMU4_E4_FORMAT "E4B0"

#define EMU4_NAME_OFFSET 2
#define EMU4_MAX_SAMPLES 1000

#define CHUNK_NAME_IS(chunk,n) (strncmp((chunk)->name, n, CHUNK_NAME_LEN) == 0)

struct chunk
{
  char name[CHUNK_NAME_LEN];
  uint32_t size;
  char data[];
};

static const struct option options[] = {
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
chunk_check_name (struct chunk *chunk, const char *name)
{
  return strncmp (chunk->name, name, CHUNK_NAME_LEN);
}

static uint32_t
chunk_get_size (struct chunk *chunk)
{
  return be32toh (chunk->size);
}

static void
chunk_print (struct chunk *chunk)
{
  emu_print (1, 0, "chunk %.4s (%ld B)\n", chunk->name,
	     chunk_get_size (chunk));
}

static void
chunk_print_with_name (struct chunk *chunk)
{
  emu_print (1, 0, "chunk %.4s: %.16s (%ld B)\n", chunk->name,
	     &chunk->data[EMU4_NAME_OFFSET], chunk_get_size (chunk));
}

static int
emu4_process_file (struct emu_file *file, int ext_mode)
{
  char *fdata;
  uint32_t size, total_size, chunk_size, new_size;
  struct chunk *chunk;
  struct emu3_sample *sample;
  int sample_index;
  uint32_t sample_start, sample_len;

  chunk = (struct chunk *) file->raw;
  if (!CHUNK_NAME_IS (chunk, EMU4_FORM_TAG))
    {
      goto cleanup;
    }

  chunk_print (chunk);
  total_size = chunk_get_size (chunk);

  if (strncmp (chunk->data, EMU4_E4_FORMAT, strlen (EMU4_E4_FORMAT)))
    {
      emu_error ("Unexpected format");
      goto cleanup;
    }

  chunk = (struct chunk *) &chunk->data[strlen (EMU4_E4_FORMAT)];	//EB40
  chunk_size = chunk_get_size (chunk);

  size = 0;
  sample_index = 1;
  while (1)
    {
      if (CHUNK_NAME_IS (chunk, EMU4_E4P1_TAG))
	{
	  chunk_print_with_name (chunk);
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_E3S1_TAG))
	{
	  chunk_print_with_name (chunk);
	  sample = (struct emu3_sample *) &chunk->data[EMU4_NAME_OFFSET];
	  emu3_process_sample (sample, sample_index, ext_mode, 0, 0);
	  sample_index++;
	}
      else if (CHUNK_NAME_IS (chunk, EMU4_E4S1_TAG))
	{
	  chunk_print_with_name (chunk);
	}
      else
	{
	  chunk_print (chunk);
	}

      if (sample_index == EMU4_MAX_SAMPLES)
	{
	  break;
	}

      new_size = size + sizeof (struct chunk) + chunk_size;
      if (new_size < size)
	{			//overflow
	  break;
	}
      size = new_size;

      if (size >= total_size || size >= file->fsize)
	{
	  break;
	}

      chunk = (struct chunk *) &chunk->data[chunk_size];
      chunk_size = chunk_get_size (chunk);

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
  int xflg = 0, errflg = 0;
  int ext_mode = 0;
  int long_index = 0;
  int err = EXIT_SUCCESS;
  const char *bank_filename;

  while ((opt = getopt_long (argc, argv, "hvxX", options, &long_index)) != -1)
    {
      switch (opt)
	{
	case 'h':
	  print_help (argv[0]);
	  exit (EXIT_SUCCESS);
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
    bank_filename = argv[optind];
  else
    errflg++;

  if (errflg > 0)
    {
      print_help (argv[0]);
      exit (EXIT_FAILURE);
    }

  struct emu_file *file = emu_open_file (bank_filename);
  if (!file)
    exit (EXIT_FAILURE);

  if (emu4_process_file (file, ext_mode))
    err = EXIT_FAILURE;

  exit (err);
}
