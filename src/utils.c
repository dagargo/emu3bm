/*
 *   utils.c
 *   Copyright (C) 2021 David García Goñi <dagargo@gmail.com>
 *
 *   This file is part of Overwitch.
 *
 *   Overwitch is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Overwitch is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Overwitch. If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

static const gchar *NOTE_NAMES[] = {
  "A-1",			//Note 0
  "A#-1",
  "B-1",
  "C0",
  "C#0",
  "D0",
  "D#0",
  "E0",
  "F0",
  "F#0",
  "G0",
  "G#0",
  "A0",
  "A#0",
  "B0",
  "C1",
  "C#1",
  "D1",
  "D#1",
  "E1",
  "F1",
  "F#1",
  "G1",
  "G#1",
  "A1",
  "A#1",
  "B1",
  "C2",
  "C#2",
  "D2",
  "D#2",
  "E2",
  "F2",
  "F#2",
  "G2",
  "G#2",
  "A2",
  "A#2",
  "B2",
  "C3",
  "C#3",
  "D3",
  "D#3",
  "E3",
  "F3",
  "F#3",
  "G3",
  "G#3",
  "A3",
  "A#3",
  "B3",
  "C4",
  "C#4",
  "D4",
  "D#4",
  "E4",
  "F4",
  "F#4",
  "G4",
  "G#4",
  "A4",
  "A#4",
  "B4",
  "C5",
  "C#5",
  "D5",
  "D#5",
  "E5",
  "F5",
  "F#5",
  "G5",
  "G#5",
  "A5",
  "A#5",
  "B5",
  "C6",
  "C#6",
  "D6",
  "D#6",
  "E6",
  "F6",
  "F#6",
  "G6",
  "G#6",
  "A6",
  "A#6",
  "B6",
  "C7",
  "C#7",			//The notes from here on appear in some places. The sampler shows their names although it can not set these values to anything higher than C7.
  "D7",
  "D#7",
  "E7",
  "F7",
  "F#7",
  "G7",
  "G#7",
  "A7",
  "A#7",
  "B7",
  "C8",
  "C#8",
  "D8",
  "D#8",
  "E8",
  "F8",
  "F#8",
  "G8",
  "G#8",
  "A8",
  "A#8",
  "B8",
  "C9",
  "C#9",
  "D9",
  "D#9",
  "E9",
  "F9",
  "F#9",
  "G9",
  "G#9",
  "A9",
  "A#9",
  "B9",
  "C10",
  "C#10",
  "D10",
  "D#10",
  "E10"				// Note 127
};

gint verbosity = 0;

struct emu_file *
emu_open_file (const gchar *name)
{
  struct emu_file *file;
  FILE *fd = fopen (name, "r");

  if (!fd)
    {
      emu_error ("Error while opening %s for input", name);
      return NULL;
    }

  file = (struct emu_file *) malloc (sizeof (struct emu_file));

  file->name = name;
  file->raw = malloc (EMU3_MEM_SIZE);
  file->size = fread (file->raw, 1, EMU3_MEM_SIZE, fd);
  fclose (fd);

  return file;
}

void
emu_close_file (struct emu_file *file)
{
  free (file->raw);
  free (file);
}

gint
emu_write_file (struct emu_file *file)
{
  gint err = 0;
  FILE *fd = fopen (file->name, "w");
  if (!fd)
    {
      emu_error ("Can't write file");
      return EXIT_FAILURE;
    }

  if (fwrite (file->raw, file->size, 1, fd) != 1)
    {
      emu_error ("Unexpected written bytes amount");
      err = EXIT_FAILURE;
    }

  fclose (fd);
  return err;
}

struct emu_file *
emu_init_file (const gchar *name)
{
  struct emu_file *file = malloc (sizeof (struct emu_file));
  file->name = name;
  file->size = 0;
  file->raw = malloc (EMU3_MEM_SIZE);
  return file;
}

gint
emu_reverse_note_search (gchar *note_name)
{
  for (gint i = 0; i < EMU3_NOTES; i++)
    if (strcasecmp (NOTE_NAMES[i], note_name) == 0)
      return i;

  return -1;
}

const gchar *
emu_get_note_name (guint8 note_num)
{
  if (note_num > 127)
    return "?";
  return NOTE_NAMES[note_num];
}

gchar *
emu3_str_to_emu3name (const gchar *src)
{
  gsize len = strlen (src);
  if (len > EMU3_NAME_SIZE)
    len = EMU3_NAME_SIZE;

  gchar *emu3name = strndup (src, len);

  gchar *c = emu3name;
  for (gint i = 0; i < len; i++, c++)
    if (*c < 32 || *c >= 127)
      *c = '?';

  return emu3name;
}

void
emu3_cpystr (gchar *dst, const gchar *src)
{
  gsize len = strlen (src);
  if (len > EMU3_NAME_SIZE)
    len = EMU3_NAME_SIZE;

  memcpy (dst, src, EMU3_NAME_SIZE);
  memset (&dst[len], ' ', EMU3_NAME_SIZE - len);
}

void
emu_print_help (gchar *executable_path, const gchar *name,
		const struct option options[])
{
  gchar *exec_name;
  const struct option *option;

  fprintf (stderr, "%s\n", name);
  exec_name = basename (executable_path);
  fprintf (stderr, "Usage: %s [options] bank\n", exec_name);
  fprintf (stderr, "Options:\n");
  option = options;
  while (option->name)
    {
      fprintf (stderr, "  -%c, --%s", option->val, option->name);
      if (option->has_arg)
	{
	  fprintf (stderr, " value");
	}
      fprintf (stderr, "\n");
      option++;
    }
}

gchar *
emu_filename_to_filename_wo_ext (const gchar *filename_, const gchar **ext)
{
  gchar *filename = strdup (filename_);
  gchar *e = strrchr (filename, '.');
  if (e)
    {
      *e = 0;
      e++;
      if (ext)
	{
	  *ext = e;
	}
    }
  return filename;
}

gint
get_positive_int (gchar *str)
{
  gchar *endstr;
  gint value = (gint) strtol (str, &endstr, 10);

  if (errno || endstr == str || *endstr != '\0')
    {
      emu_error ("Value '%s' not valid", str);
      value = -1;
    }
  else
    {
      if (value < 0)
	{
	  emu_error ("Value '%s' is negative", str);
	}
    }

  return value;
}

gint
get_positive_int_in_range (gchar *str, gint min, gint max)
{
  gint v = get_positive_int (str);

  if (v < 0)
    {
      return v;
    }
  else if (v < min)
    {
      emu_error ("Value '%d' not allowed (%d < %d)", v, v, min);
      v = -1;
    }
  else if (v > max)
    {
      emu_error ("Value '%d' not allowed (%d > %d)", v, v, max);
      v = -1;
    }

  return v;
}
