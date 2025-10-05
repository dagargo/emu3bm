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

struct emu3_envelope
{
  uint8_t attack;
  uint8_t hold;
  uint8_t decay;
  uint8_t sustain;
  uint8_t release;
};

struct emu3_preset_zone
{
  uint8_t original_key;
  uint8_t sample_id_lsb;
  uint8_t sample_id_msb;
  int8_t parameter_a;
  struct emu3_envelope vca_envelope;
  uint8_t lfo_rate;
  uint8_t lfo_delay;
  uint8_t lfo_variation;
  uint8_t vcf_cutoff;
  uint8_t vcf_q;		// 10000000b/0x80 bit flags that rt-vcf-noteon-q is enabled
  uint8_t vcf_envelope_amount;
  struct emu3_envelope vcf_envelope;
  struct emu3_envelope aux_envelope;
  int8_t aux_envelope_amount;
  uint8_t aux_envelope_dest;
  int8_t vel_to_vca_level;
  int8_t vel_to_vca_attack;
  int8_t vel_to_vcf_cutoff;
  int8_t vel_to_pitch;
  int8_t vel_to_aux_env;
  int8_t vel_to_vcf_q;
  int8_t vel_to_vcf_attack;
  int8_t vel_to_sample_start;
  int8_t vel_to_vca_pan;
  int8_t lfo_to_pitch;
  int8_t lfo_to_vca;
  int8_t lfo_to_cutoff;
  int8_t lfo_to_pan;
  int8_t vca_level;
  int8_t note_tuning;		// -64 to 64 for -100cents to 100cents
  int8_t vcf_tracking;
  uint8_t note_on_delay;	// 0x00 to 0xFF for 0.00 to 1.53s
  int8_t vca_pan;
  uint8_t vcf_type_lfo_shape;

  // `rt_enable_flags`
  // The realtime controls can be disabled on the unit (via Dynamic
  // processing, Realtime enable). These are normally all enabled (0xff)
  // but can be turned off.
  //
  // 0000 0000
  // |||| |||^pitch       0x01
  // |||| ||^vcf cutoff   0x02
  // |||| |^vca level     0x04
  // |||| ^lfo->pitch     0x08
  // |||^lfo->vcf cutoff  0x10
  // ||^lfo->vca          0x20
  // |^attack             0x40
  // ^pan                 0x80
  uint8_t rt_enable_flags;	// 0xff

  // Various settings are encoded into this last byte of the structure. The
  // bits set the following settings:
  // 'chorus (off/on)',
  // 'disable loop (off/on)',
  // 'disable side (off/left/right)' (2 bits),
  // 'keyboard env. mode (gate/trigger)',
  // 'solo (off/on)',
  // 'nontranspose (off/on)'
  //
  // There unused bit 0 is unknown at this point.
  //
  //  1010 0000 = off, disable side right
  //  1010 1000 = on, disable side right
  //  0110 1000 = disable side left
  //  |||| ||^nontranspose = 1
  //  |||| |^env mode trigger = 1 (else gate)
  //  |||| ^chorus on = 1
  //  |||^ solo on = 1
  //  ||^disable loop on = 1
  //  |^disable left = 1
  //  ^disable right
  uint8_t flags;
};

// 1 0110
// env mode trigger, solo on, notranspose on

// 1 0010
// env mode gate, solo on

int emu3_add_sample (struct emu_file *file, char *path, int *sample_num);

int emu3_add_preset (struct emu_file *file, char *preset_name,
		     int *preset_num);

int
emu3_add_preset_zone (struct emu_file *file, int preset_num, int sample_num,
		      struct emu3_zone_range *zone_range,
		      struct emu3_preset_zone **zone);

int emu3_del_preset_zone (struct emu_file *, int, int);

int emu3_process_bank (struct emu_file *, int, int, char *, int, int,
		       int, int, int);

int emu3_create_bank (const char *, const char *);

const char *emu3_get_err (int);

struct emu_file *emu3_open_file (const char *filename);

int emu3_add_sfz (struct emu_file *file, const char *sfz_path);
