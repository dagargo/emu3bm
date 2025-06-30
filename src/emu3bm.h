/*
 *   emu3bm.h
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

#include "sample.h"
#include "utils.h"
#include "../config.h"

#define DEVICE_ESI2000 "esi2000"
#define DEVICE_EMU3X "emu3x"

struct emu3_zone_range
{
  unsigned char layer;
  unsigned char original_key;
  unsigned char lower_key;
  unsigned char higher_key;
};

int emu3_add_sample (struct emu_file *, char *, int);

int emu3_add_preset (struct emu_file *, char *);

int emu3_add_preset_zone (struct emu_file *, int, int,
			  struct emu3_zone_range *);

int emu3_del_preset_zone (struct emu_file *, int, int);

int emu3_process_bank (struct emu_file *, int, int, char *, int, int, int,
		       int, int);

int emu3_create_bank (const char *, const char *);

const char *emu3_get_err (int);

struct emu_file *emu3_open_file (const char *name);
