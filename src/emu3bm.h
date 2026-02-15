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

// This does not represent a native structure.

struct emu_zone_range
{
  guint8 layer;
  guint8 original_key;
  guint8 lower_key;
  guint8 higher_key;
};

struct emu3_envelope
{
  guint8 attack;
  guint8 hold;
  guint8 decay;
  guint8 sustain;
  guint8 release;
};

struct emu3_preset_zone
{
  guint8 original_key;
  guint8 sample_id_lsb;
  guint8 sample_id_msb;
  gint8 parameter_a;
  struct emu3_envelope vca_envelope;
  guint8 lfo_rate;
  guint8 lfo_delay;
  guint8 lfo_variation;
  guint8 vcf_cutoff;
  guint8 vcf_q;			// 10000000b/0x80 bit flags that rt-vcf-noteon-q is enabled
  gint8 vcf_envelope_amount;
  struct emu3_envelope vcf_envelope;
  struct emu3_envelope aux_envelope;
  gint8 aux_envelope_amount;
  guint8 aux_envelope_dest;
  gint8 vel_to_aux_env;
  gint8 vel_to_vca_level;
  gint8 vel_to_vca_attack;
  gint8 vel_to_pitch;
  gint8 vel_to_pan;
  gint8 vel_to_vcf_cutoff;
  gint8 vel_to_vcf_q;
  gint8 vel_to_vcf_attack;
  gint8 vel_to_sample_start;
  gint8 lfo_to_pitch;
  gint8 lfo_to_vca;
  gint8 lfo_to_cutoff;
  gint8 lfo_to_pan;
  gint8 vca_level;
  gint8 note_tuning;		// -64 to 64 for -100cents to 100cents
  gint8 vcf_tracking;
  guint8 note_on_delay;		// 0x00 to 0xFF for 0.00 to 1.53s
  gint8 vca_pan;
  guint8 vcf_type_lfo_shape;

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
  guint8 rt_enable_flags;	// 0xff

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
  guint8 flags;
};

// 1 0110
// env mode trigger, solo on, notranspose on

// 1 0010
// env mode gate, solo on

gint emu3_add_sample (struct emu_file *file, gchar * sample_path,
		      gint * sample_num, gboolean * mono, guint32 * frames);

gint emu3_add_preset (struct emu_file *file, gchar * preset_name,
		      gint * preset_num);

gint
emu3_add_preset_zone (struct emu_file *file, gint preset_num, gint sample_num,
		      struct emu_zone_range *zone_range,
		      struct emu3_preset_zone **zone);

gint emu3_del_preset_zone (struct emu_file *, gint, gint);

gint emu3_process_bank (struct emu_file *, gint, gint, gchar *, gint, gint,
			gint, gint, gint);

gint emu3_create_bank (const gchar *, const gchar *);

const gchar *emu3_get_err (gint);

struct emu_file *emu3_open_file (const gchar * filename);

gint emu3_write_file (struct emu_file *file);

gint emu3_add_sfz (struct emu_file *file, const gchar * sfz_path);
