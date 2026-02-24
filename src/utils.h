/*
 *   utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <getopt.h>
#include <glib.h>
#include <libgen.h>
#include <stdint.h>
#include <unistd.h>

#define EMU3_MEM_SIZE 0x08000000	//128 MiB
#define EMU3_NAME_SIZE 16
#define SAMPLE_EXT ".wav"
#define EMU3_NOTES 88		// 0x58

#define EMU3_MIDI_NOTE_OFFSET 21
#define EMU3_LOWEST_MIDI_NOTE EMU3_MIDI_NOTE_OFFSET
#define EMU3_HIGHEST_MIDI_NOTE (EMU3_NOTES - 1 + EMU3_MIDI_NOTE_OFFSET)

struct emu_file
{
  const gchar *name;
  gchar *raw;
  gsize size;
};

#define emu_print(level, indent, ...) { \
		if (level <= verbosity) { \
			for (gint i = 0; i < indent; i++) \
				fprintf(stdout, "  "); \
			fprintf(stdout, __VA_ARGS__); \
		} \
	}

#define emu_debug(level, format, ...) { \
                if (level <= verbosity) { \
                        fprintf(stderr, "DEBUG:" __FILE__ ":%d:(%s): " format "\n", __LINE__, __FUNCTION__, ## __VA_ARGS__); \
                } \
        }

#define emu_error(format, ...) { \
                gint tty = isatty(fileno(stderr)); \
                const gchar * color_start = tty ? "\x1b[31m" : ""; \
                const gchar * color_end = tty ? "\x1b[m" : ""; \
                fprintf(stderr, "%sERROR:" __FILE__ ":%d:(%s): " format "%s\n", color_start, __LINE__, __FUNCTION__, ## __VA_ARGS__, color_end); \
        }

#define emu_warn(format, ...) { \
                gint tty = isatty(fileno(stderr)); \
                const gchar * color_start = tty ? "\x1b[33m" : ""; \
                const gchar * color_end = tty ? "\x1b[m" : ""; \
                fprintf(stderr, "%sWARN :" __FILE__ ":%d:(%s): " format "%s\n", color_start, __LINE__, __FUNCTION__, ## __VA_ARGS__, color_end); \
        }

extern gint verbosity;

const gchar *emu_get_err (gint);

struct emu_file *emu_open_file (const gchar *);

void emu_close_file (struct emu_file *);

gint emu_write_file (struct emu_file *);

struct emu_file *emu_init_file ();

gint emu_reverse_note_search (gchar *);

const gchar *emu_get_note_name (uint8_t);

void emu_print_help (gchar * executable_path, const gchar * name,
		     const struct option options[]);

gchar *emu3_str_to_emu3name (const gchar * src);

void emu3_cpystr (gchar * dst, const gchar * src);

gchar *emu_filename_to_filename_wo_ext (const gchar * file,
					const gchar ** ext);

gint get_positive_int (gchar * str);

gint get_positive_int_in_range (gchar * str, gint min, gint max);

#endif
