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
#define CONTENT_CHUNK_DATA_LEN 24
#define CONTENT_CHUNK_LEN (sizeof(struct chunk) + CONTENT_CHUNK_DATA_LEN)

#define EMU4_FORM_TAG "FORM"
#define EMU4_E4_FORMAT "E4B0"
#define EMU4_TOC_TAG "TOC1"
#define EMU4_E3_SAMPLE_TAG "E3S1"
#define EMU4_E3_SAMPLE_OFFSET (sizeof (struct chunk) + 2)

struct chunk
{
  char name[CHUNK_NAME_LEN];
  unsigned int len;
  char data[];
};

static const struct option options[] = {
  {"extract-samples", 0, NULL, 'x'},
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

int
check_chunk_name (struct chunk *chunk, const char *name)
{
  int v = strncmp (chunk->name, name, CHUNK_NAME_LEN);
  if (v)
    emu_error ("Unexpected chunk name %s", name);
  return v;
}

int
emu4_process_file (struct emu_file *file, int ext_mode)
{
  char *fdata;
  uint32_t chunk_len;
  struct chunk *chunk;
  struct chunk *content_chunk, *sample_chunk;
  int i, sample_index;

  chunk = (struct chunk *) file->raw;
  if (check_chunk_name (chunk, EMU4_FORM_TAG))
    goto cleanup;

  chunk_len = be32toh (chunk->len);
  emu_print (1, 0, "FORM: %ld\n", chunk_len);

  if (strncmp (chunk->data, EMU4_E4_FORMAT, strlen (EMU4_E4_FORMAT)))
    {
      emu_error ("Unexpected format");
      goto cleanup;
    }

  chunk = (struct chunk *) &chunk->data[strlen (EMU4_E4_FORMAT)];	//EB40
  if (check_chunk_name (chunk, EMU4_TOC_TAG))
    goto cleanup;

  chunk_len = be32toh (chunk->len);
  emu_print (1, 0, "%s: %ld --\n", EMU4_TOC_TAG, chunk_len);

  i = 0;
  content_chunk = (struct chunk *) chunk->data;
  sample_index = 0;
  while (i < chunk_len)
    {
      unsigned int sample_start =
	be32toh (*(unsigned int *) content_chunk->data);
      unsigned int sample_len =
	be32toh (content_chunk->len) + EMU4_E3_SAMPLE_OFFSET;
      emu_print (1, 0, "%.4s: %.16s @ 0x%08x, 0x%08x\n", content_chunk->name,
		 &content_chunk->data[6], sample_start, sample_len);

      if (strcmp (content_chunk->name, EMU4_E3_SAMPLE_TAG) == 0)
	{
	  sample_chunk = (struct chunk *) &file->raw[sample_start];
	  struct emu3_sample *sample =
	    (struct emu3_sample *) &sample_chunk->data[2];
	  emu3_process_sample (sample, sample_index + 1,
			       be32toh (sample_chunk->len), ext_mode);
	  sample_index++;
	}

      content_chunk =
	(struct chunk *) &content_chunk->data[CONTENT_CHUNK_DATA_LEN];
      i += CONTENT_CHUNK_LEN;
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
	  ext_mode = 1;
	  break;
	case 'X':
	  xflg++;
	  ext_mode = 2;
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
