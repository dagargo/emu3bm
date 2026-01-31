/*
 *   sfz.h
 *   Copyright (C) 2025 David García Goñi <dagargo@gmail.com>
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

#include "utils.h"

struct emu_sfz_context
{
  gint preset_num;
  const gchar *sfz_dir;
};

void emu3_sfz_region_add (struct emu_file *file,
			  struct emu_sfz_context *emu_sfz_context,
			  GHashTable * global_opcodes,
			  GHashTable * group_opcodes,
			  GHashTable * region_opcodes);

extern void sfz_parser_set_context (struct emu_file *file,
				    struct emu_sfz_context *emu_sfz_context);
