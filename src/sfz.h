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

struct emu_velocity_range_map
{
  guint8 low;
  guint8 high;
  gint preset_num;
};

struct emu_sfz_context
{
  struct emu_file *file;
  const gchar *preset_name;
  gint velocity_layer_num;
  gint region_num;
  const gchar *sfz_dir;
  struct emu_velocity_range_map emu_velocity_range_maps[EMU3_NOTES];
  GHashTable *global_opcodes;
  GHashTable *group_opcodes;
  GHashTable *region_opcodes;
};

void emu3_sfz_region_add (struct emu_sfz_context *esctx);

void sfz_parser_set_context (struct emu_sfz_context *esctx);
