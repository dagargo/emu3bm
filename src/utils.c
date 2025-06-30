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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

static const char *EMU_ERROR_STR[] = {
  NULL, "Bank is full", "Sample neither mono nor stereo",
  "Error while opening sample file",
  "No more samples allowed",
  "No more presetc allowed",
  "Error while opening bank file",
  "Error while writing bank file",
};

static const char *NOTE_NAMES[] = {
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

int verbosity = 0;

const char *
emu_get_err (int error)
{
  return EMU_ERROR_STR[error];
}

struct emu_file *
emu_open_file (const char *name)
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
  file->raw = malloc (MEM_SIZE);
  file->size = fread (file->raw, 1, MEM_SIZE, fd);
  fclose (fd);

  return file;
}

void
emu_close_file (struct emu_file *file)
{
  free (file->raw);
  free (file);
}

int
emu_write_file (struct emu_file *file)
{
  int err = 0;
  FILE *fd = fopen (file->name, "w");
  if (!fd)
    {
      return ERR_OPEN_BANK;
    }

  if (fwrite (file->raw, file->size, 1, fd) != 1)
    {
      emu_error ("Unexpected written bytes amount.");
      err = ERR_WRITE_BANK;
    }

  fclose (fd);
  return err;
}

struct emu_file *
emu_init_file (const char *name)
{
  struct emu_file *file = malloc (sizeof (struct emu_file));
  file->name = name;
  file->size = 0;
  file->raw = malloc (MEM_SIZE);
  return file;
}

int
emu_reverse_note_search (char *note_name)
{
  for (int i = 0; i < NOTES; i++)
    if (strcasecmp (NOTE_NAMES[i], note_name) == 0)
      return i;

  return -1;
}

const char *
emu_get_note_name (unsigned char note_num)
{
  if (note_num > 127)
    return "?";
  return NOTE_NAMES[note_num];
}
