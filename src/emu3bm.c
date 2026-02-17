/*
 *   emu3bm.c
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

#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "emu3bm.h"
#include "sfz.h"
#include "sfz.tab.h"
#include "utils.h"

#define FORMAT_SIZE 16
#define BANK_PARAMETERS 3
#define SAMPLE_ADDR_START_EMU_3X 0x1bd2
#define SAMPLE_ADDR_START_EMU_THREE 0x204
#define PRESET_OFFSET_EMU_THREE 0x1a6fe
#define PRESET_START_EMU_3X 0x2b72
#define PRESET_START_EMU_THREE 0x74a
#define SAMPLE_OFFSET 0x400000	//This is also the max sample length in bytes
#define MAX_SAMPLES_EMU_3X 999
#define MAX_SAMPLES_EMU_THREE 99
#define PRESET_SIZE_ADDR_START_EMU_3X 0x17ca
#define PRESET_SIZE_ADDR_START_EMU_THREE 0x6c
#define MAX_PRESETS_EMU_3X 0x100
#define MAX_PRESETS_EMU_THREE 100

#define RT_CONTROLS_SIZE 10
#define RT_CONTROLS_FS_SIZE 2
#define PRESET_UNKNOWN_0_SIZE 16
#define PRESET_UNKNOWN_1_SIZE 2

#define ESI_32_V3_DEF      "EMU SI-32 v3   "
#define EMULATOR_3X_DEF    "EMULATOR 3X    "
#define EMULATOR_THREE_DEF "EMULATOR THREE "

#define EMPTY_BANK_TEMPLATE "res/empty_bank_"

#define WRITE_BUFLEN 4096

#define EMU3_BLOCK_SIZE 512

#define DEFAULT_CUTOFF_U8 0xef

#define EMU3_BANK(f) ((struct emu3_bank *) ((f)->raw))

extern void yyset_in (FILE * _in_str);

struct emu3_bank
{
  gchar format[FORMAT_SIZE];
  gchar name[EMU3_NAME_SIZE];
  guint32 objects;
  guint32 padding[3];
  guint32 next_preset;
  guint32 next_sample;
  guint32 unknown_0;
  guint32 preset_blocks;
  guint32 sample_blocks;
  guint32 unknown_1;
  guint32 total_blocks;
  gchar name_copy[EMU3_NAME_SIZE];
  guint32 selected_preset;
  guint32 parameters[BANK_PARAMETERS];
};

struct emu3_preset_note_zone
{
  guint8 options_lsb;
  guint8 options_msb;
  guint8 pri_zone;
  guint8 sec_zone;
};

struct emu3_preset
{
  gchar name[EMU3_NAME_SIZE];
  gint8 rt_controls[RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE];
  gint8 unknown_0[PRESET_UNKNOWN_0_SIZE];
  gint8 pitch_bend_range;
  gint8 velocity_range_pri_low;
  gint8 velocity_range_pri_high;
  gint8 velocity_range_sec_low;
  gint8 velocity_range_sec_high;
  gint8 link_preset_lsb;
  gint8 link_preset_msb;
  gint8 unknown_1[PRESET_UNKNOWN_1_SIZE];
  gint8 note_zones;
  guint8 note_zone_mappings[EMU3_NOTES];
};

static const gint8 DEFAULT_RT_CONTROLS[RT_CONTROLS_SIZE +
				       RT_CONTROLS_FS_SIZE] =
  { 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 8 };

static const gchar *RT_CONTROLS_SRC[] = {
  "Pitch Control",
  "Mod Control",
  "Pressure Control",
  "Pedal Control",
  "MIDI A Control",
  "MIDI B Control"
};

static const gint RT_CONTROLS_SRC_SIZE =
  sizeof (RT_CONTROLS_SRC) / sizeof (gchar *);

static const gchar *RT_CONTROLS_DST[] = {
  "Off",
  "Pitch",
  "VCF Cutoff",
  "VCA Level",
  "LFO -> Pitch",
  "LFO -> Cutoff",
  "LFO -> VCA",
  "Pan",
  "Attack",
  "Crossfade",
  "VCF NoteOn Q"
};

static const gint RT_CONTROLS_DST_SIZE =
  sizeof (RT_CONTROLS_DST) / sizeof (gchar *);

static const gchar *RT_CONTROLS_FS_SRC[] = {
  "Footswitch 1",
  "Footswitch 2",
};

static const gint RT_CONTROLS_FS_SRC_SIZE =
  sizeof (RT_CONTROLS_FS_SRC) / sizeof (gchar *);

static const gchar *RT_CONTROLS_FS_DST[] = {
  "Off",
  "Sustain",
  "Cross-Switch",
  "Unused 1",
  "Unused 2",
  "Unused 3",
  "Unused A",
  "Unused B",
  "Preset Increment",
  "Preset Decrement"
};

static const gint RT_CONTROLS_FS_DST_SIZE =
  sizeof (RT_CONTROLS_FS_DST) / sizeof (gint8 *);

static const gchar *LFO_SHAPE[] = {
  "triangle",
  "sine",
  "sawtooth",
  "square"
};

static const gchar *VCF_TYPE[] = {
  "2 Pole Lowpass",
  "4 Pole Lowpass",
  "6 Pole Lowpass",
  "2nd Ord Hipass",
  "4th Ord Hipass",
  "2nd O Bandpass",
  "4th O Bandpass",
  "Contrary BandP",
  "Swept EQ 1 oct",
  "Swept EQ 2->1",
  "Swept EQ 3->1",
  "Phaser 1",
  "Phaser 2",
  "Bat-Phaser",
  "Flanger Lite",
  "Vocal Ah-Ay-Ee",
  "Vocal Oo-Ah",
  "Bottom Feeder",
  "ESi/E3x Lopass",
  "Unknown"
};

static const gchar *AUX_ENV_DST[] = {
  "Off",
  "Pitch",
  "Pan",
  "LFO rate",
  "LFO -> Pitch",
  "LFO -> VCA",
  "LFO -> VCF",
  "LFO -> Pan"
};

static const gint VCF_TYPE_SIZE = sizeof (VCF_TYPE) / sizeof (gchar *);

static const gchar *OFF_ON[] = { "Off", "On" };

static const gchar *SIDES_DISABLED[] = { "left", "off", "right" };

// Percentage -100..100 from 0x00 to 0x7F
static const gint TABLE_PERCENTAGE_SIGNED[] = {
  -100, -99, -97, -96, -94, -93, -91, -90, -88, -86, -85, -83, -82, -80, -79,
  -77, -75, -74, -72, -71, -69, -68, -66, -65, -63, -61, -60, -58, -57, -55,
  -54, -52, -50, -49, -47, -46, -44, -43, -41, -40, -38, -36, -35, -33, -32,
  -30, -29, -27, -25, -24, -22, -21, -19, -18, -16, -15, -13, -11, -10, -8,
  -7, -5, -4, -2, 0, 1, 3, 4, 6, 7, 9, 11, 12, 14, 15,
  17, 19, 20, 22, 23, 25, 26, 28, 30, 31, 33, 34, 36, 38, 39,
  41, 42, 44, 46, 47, 49, 50, 52, 53, 55, 57, 58, 60, 61, 63,
  65, 66, 68, 69, 71, 73, 74, 76, 77, 79, 80, 82, 84, 85, 87,
  88, 90, 92, 93, 95, 96, 98, 100
};

// LFO rate table.
static const gfloat TABLE_LFO_RATE_FLOAT[] = {
  0.08f, 0.11f, 0.15f, 0.18f, 0.21f, 0.25f, 0.28f, 0.32f, 0.35f,
  0.39f, 0.42f, 0.46f, 0.50f, 0.54f, 0.58f, 0.63f, 0.67f, 0.71f,
  0.76f, 0.80f, 0.85f, 0.90f, 0.94f, 0.99f, 1.04f, 1.10f, 1.15f,
  1.20f, 1.25f, 1.31f, 1.37f, 1.42f, 1.48f, 1.54f, 1.60f, 1.67f,
  1.73f, 1.79f, 1.86f, 1.93f, 2.00f, 2.07f, 2.14f, 2.21f, 2.29f,
  2.36f, 2.44f, 2.52f, 2.60f, 2.68f, 2.77f, 2.85f, 2.94f, 3.03f,
  3.12f, 3.21f, 3.31f, 3.40f, 3.50f, 3.60f, 3.70f, 3.81f, 3.91f,
  4.02f, 4.13f, 4.25f, 4.36f, 4.48f, 4.60f, 4.72f, 4.84f, 4.97f,
  5.10f, 5.23f, 5.37f, 5.51f, 5.65f, 5.79f, 5.94f, 6.08f, 6.24f,
  6.39f, 6.55f, 6.71f, 6.88f, 7.04f, 7.21f, 7.39f, 7.57f, 7.75f,
  7.93f, 8.12f, 8.32f, 8.51f, 8.71f, 8.92f, 9.13f, 9.34f, 9.56f,
  9.78f, 10.00f, 10.23f, 10.47f, 10.71f, 10.95f, 11.20f, 11.46f, 11.71f,
  11.98f, 12.25f, 12.52f, 12.80f, 13.09f, 13.38f, 13.68f, 13.99f, 14.30f,
  14.61f, 14.93f, 15.26f, 15.60f, 15.94f, 16.29f, 16.65f, 17.01f, 17.38f,
  17.76f, 18.14f
};

// VCF cutoff frequency table.
static const gint TABLE_VCF_CUTOFF_FREQUENCY[] = {
  26, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 39, 40, 41, 42, 44, 45, 47, 48, 50,
  51, 53, 55, 56, 58, 60, 62, 64, 66, 68, 70,
  72, 75, 77, 80, 82, 85, 87, 90, 93, 96, 99,
  102, 106, 109, 112, 116, 120, 124, 128, 132, 136, 140,
  145, 149, 154, 159, 164, 169, 175, 180, 186, 192, 198,
  204, 211, 217, 224, 231, 239, 246, 254, 262, 271, 279,
  288, 297, 307, 316, 327, 337, 348, 359, 370, 382, 394,
  407, 419, 433, 447, 461, 475, 491, 506, 522, 539, 556,
  574, 592, 611, 630, 650, 671, 692, 714, 737, 760, 784,
  809, 835, 861, 889, 917, 946, 976, 1007, 1039, 1072, 1106,
  1141, 1178, 1215, 1254, 1294, 1335, 1377, 1421, 1466, 1512, 1560,
  1610, 1661, 1714, 1768, 1825, 1882, 1942, 2004, 2068, 2133, 2201,
  2271, 2343, 2417, 2494, 2573, 2655, 2739, 2826, 2916, 3009, 3104,
  3203, 3304, 3409, 3518, 3629, 3744, 3863, 3986, 4112, 4243, 4378,
  4517, 4660, 4808, 4960, 5118, 5280, 5448, 5621, 5799, 5983, 6173,
  6368, 6570, 6779, 6994, 7216, 7444, 7680, 7924, 8175, 8434, 8702,
  8978, 9262, 9556, 9859, 10171, 10493, 10826, 11169, 11522, 11887, 12264,
  12652, 13053, 13466, 13892, 14332, 14785, 15253, 15736, 16233, 16747, 17276,
  17823, 18386, 18967, 19566, 20185, 20822, 21480, 22158, 22858, 23580, 24324,
  25091, 25883, 26699, 27541, 28409, 29305, 30228, 31181, 32163, 33176, 34220,
  35297, 36407, 37553, 38734, 39951, 41207, 42502, 43836, 45213, 46632, 48095,
  49604, 51160, 52763, 54417, 56121, 57879, 59691, 61559, 63484, 65469, 67515,
  69625, 71799, 74040
};

// 0..21.69
static gfloat TABLE_TIME_21_69_FLOAT[] = {
  0.00f, 0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.08f,
  0.09f, 0.10f, 0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f,
  0.18f, 0.19f, 0.20f, 0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f,
  0.28f, 0.30f, 0.32f, 0.34f, 0.36f, 0.38f, 0.40f, 0.42f, 0.44f,
  0.47f, 0.49f, 0.52f, 0.54f, 0.57f, 0.60f, 0.63f, 0.66f, 0.69f,
  0.73f, 0.76f, 0.80f, 0.84f, 0.87f, 0.92f, 0.96f, 1.00f, 1.05f,
  1.10f, 1.15f, 1.20f, 1.25f, 1.31f, 1.37f, 1.43f, 1.49f, 1.56f,
  1.63f, 1.70f, 1.77f, 1.84f, 1.92f, 2.01f, 2.09f, 2.18f, 2.27f,
  2.37f, 2.47f, 2.58f, 2.69f, 2.80f, 2.92f, 3.04f, 3.17f, 3.30f,
  3.44f, 3.59f, 3.73f, 3.89f, 4.05f, 4.22f, 4.40f, 4.58f, 4.77f,
  4.96f, 5.17f, 5.38f, 5.60f, 5.83f, 6.07f, 6.32f, 6.58f, 6.85f,
  7.13f, 7.42f, 7.72f, 8.04f, 8.37f, 8.71f, 9.06f, 9.43f, 9.81f,
  10.21f, 10.63f, 11.06f, 11.51f, 11.97f, 12.46f, 12.96f, 13.49f, 14.03f,
  14.60f, 15.19f, 15.81f, 16.44f, 17.11f, 17.80f, 18.52f, 19.26f, 20.04f,
  20.85f, 21.69f
};

// 0..163.69
static gfloat TABLE_TIME_163_69_FLOAT[] = {
  0.00f, 0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.08f,
  0.09f, 0.10f, 0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f,
  0.18f, 0.19f, 0.20f, 0.21f, 0.22f, 0.23f, 0.25f, 0.26f, 0.28f,
  0.29f, 0.32f, 0.34f, 0.36f, 0.38f, 0.41f, 0.43f, 0.46f, 0.49f,
  0.52f, 0.55f, 0.58f, 0.62f, 0.65f, 0.70f, 0.74f, 0.79f, 0.83f,
  0.88f, 0.93f, 0.98f, 1.04f, 1.10f, 1.17f, 1.24f, 1.31f, 1.39f,
  1.47f, 1.56f, 1.65f, 1.74f, 1.84f, 1.95f, 2.06f, 2.18f, 2.31f,
  2.44f, 2.59f, 2.73f, 2.89f, 3.06f, 3.23f, 3.42f, 3.62f, 3.82f,
  4.04f, 4.28f, 4.52f, 4.78f, 5.05f, 5.34f, 5.64f, 5.97f, 6.32f,
  6.67f, 7.06f, 7.46f, 7.90f, 8.35f, 8.83f, 9.34f, 9.87f, 10.45f,
  11.06f, 11.70f, 12.38f, 13.11f, 13.88f, 14.70f, 15.56f, 16.49f, 17.48f,
  18.53f, 19.65f, 20.85f, 22.13f, 23.50f, 24.97f, 26.54f, 28.24f, 30.06f,
  32.02f, 34.15f, 36.44f, 38.93f, 41.64f, 44.60f, 47.84f, 51.41f, 55.34f,
  59.70f, 64.56f, 70.03f, 76.22f, 83.28f, 91.40f, 100.87f, 112.09f, 125.65f,
  142.36f, 163.69f
};

gint
emu3_get_vcf_cutoff_frequency_from_u8 (const guint8 v)
{
  return TABLE_VCF_CUTOFF_FREQUENCY[v];
}

guint8
emu3_get_u8_from_vcf_cutoff_frequency (const gint v)
{
  if (v < TABLE_VCF_CUTOFF_FREQUENCY[0])
    {
      return 0;
    }
  for (guint8 i = 0; i < 254; i++)
    {
      if (TABLE_VCF_CUTOFF_FREQUENCY[i] <= v &&
	  v < TABLE_VCF_CUTOFF_FREQUENCY[i + 1])
	{
	  return i;
	}
    }
  return 255;
}


static inline gfloat
emu3_get_lfo_rate (const gint8 value)
{
  if (value >= 0 && value <= 127)
    return TABLE_LFO_RATE_FLOAT[value];

  return TABLE_LFO_RATE_FLOAT[0];
}

gfloat
emu3_get_time_163_69_from_u8 (guint8 v)
{
  if (v > 127)
    {
      v = 127;
    }
  return TABLE_TIME_163_69_FLOAT[v];
}

guint8
emu3_get_u8_from_time_163_69 (gfloat v)
{
  if (v < TABLE_TIME_163_69_FLOAT[0])
    {
      return 0;
    }
  for (guint8 i = 0; i < 127; i++)
    {
      if (TABLE_TIME_163_69_FLOAT[i] <= v &&
	  v < TABLE_TIME_163_69_FLOAT[i + 1])
	{
	  return i;
	}
    }
  return 127;
}

static inline gfloat
emu3_get_time_21_69 (const guint8 index)
{
  if (index <= 127)
    return TABLE_TIME_21_69_FLOAT[index];

  return 0.0f;
}

static inline gfloat
emu3_get_note_on_delay (const guint8 value)
{
  return (gfloat) value *0.006f;
}

// [-64, 0, +64] to -100, 0, 100
gfloat
emu3_get_note_tuning_from_s8 (const gint8 v)
{
  return (gfloat) v *1.5625f;
}

gint8
emu3_s8_from_get_note_tuning (const gfloat v)
{
  return v / 1.5625f;
}

// `rt_enable_flags`
static inline gint
emu3_get_is_rt_pitch_enabled (const guint8 value)
{
  return (value & 0x01) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_vcf_cutoff_enabled (const guint8 value)
{
  return (value & 0x02) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_vca_level_enabled (const guint8 value)
{
  return (value & 0x04) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_lfo_pitch_enabled (const guint8 value)
{
  return (value & 0x08) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_lfo_vcf_cutoff_enabled (const guint8 value)
{
  return (value & 0x10) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_lfo_vca_enabled (const guint8 value)
{
  return (value & 0x20) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_attack_enabled (const guint8 value)
{
  return (value & 0x40) ? 1 : 0;
}

static inline gint
emu3_get_is_rt_pan_enabled (const guint8 value)
{
  return (value & 0x80) ? 1 : 0;
}

// `zone->vcf_q`
static inline gint
emu3_get_is_rt_note_on_q_enabled (const guint8 value)
{
  return (value & 0x80) ? 1 : 0;
}

// `zone->flags`
static inline gint
emu3_get_is_chorus_enabled (const guint8 value)
{
  return (value & 0x08) ? 1 : 0;
}

// `zone->flags`
static inline gint
emu3_get_is_loop_disabled (const guint8 value)
{
  return (value & 0x20) ? 1 : 0;
}

// `zone->flags`
static inline gint
emu3_get_env_mode_trigger (const guint8 value)
{
  return (value & 0x04) ? 1 : 0;
}

// `zone->flags`
static inline gint
emu3_get_is_nontranspose_enabled (const guint8 value)
{
  return (value & 0x02) ? 1 : 0;
}

// `zone->flags`
static inline gint
emu3_get_is_solo_enabled (const guint8 value)
{
  return (value & 0x10) ? 1 : 0;
}

// `zone->flags`, returns -1 if left disabled, +1 if right disabled, else 0.
static inline gint
emu3_get_is_side_disabled (const guint8 value)
{
  if (value & 0x80)
    return 1;

  if (value & 0x40)
    return -1;

  return 0;
}

//Level: [-127, 127] -> [-100, 100]
gint
emu3_get_percent_from_s8 (const gint8 v)
{
  const gdouble percentage = (v * 100 / 127.0);
  if (percentage < 0.0)
    return (gint) (percentage - 0.5);
  return (gint) (percentage + 0.5);
}

gint8
emu3_get_s8_from_percent (gint v)
{
  return (v * 127.0) / 100.0;
}

// Level: [0x00, 0x40, 0x7f] -> [-100, 0, +100]
gint
emu3_get_percent_signed_from_s8 (gint8 v)
{
  if (v < 0)
    {
      v = 0;
    }
  return TABLE_PERCENTAGE_SIGNED[v];
}

gint8
emu3_get_s8_from_percent_signed (gint v)
{
  if (v < 0)
    {
      return 0;
    }
  for (guint8 i = 0; i < 126; i++)
    {
      if (TABLE_PERCENTAGE_SIGNED[i] <= v &&
	  v < TABLE_PERCENTAGE_SIGNED[i + 1])
	{
	  return i;
	}
    }
  return 127;
}

// [-127, 0, 127] to [-2.0, 0, 2.0]
gfloat
emu3_get_vcf_tracking_from_s8 (const gint8 v)
{
  static const gfloat range = 4.0f / 254.0f;
  static const gfloat out_min = -2.0f;
  static const gfloat in_min = -127.0f;
  const gdouble as_double = 100 * (out_min + ((gfloat) v - in_min) * range);
  const gint as_int = (gint) as_double;
  return (gfloat) as_int / 100.0f;
}

gint8
emu3_get_s8_from_vcf_tracking (const gfloat v)
{
  double aux = (v * 127) / 2.0;
  return v > 0 ? ceill (aux) : floorl (aux);
}

static void
emu3_print_envelope (struct emu3_envelope *envelope)
{
  emu_print (1, 4, "Envelope\n");
  emu_print (1, 5, "Attack:  %.02f s\n",
	     emu3_get_time_163_69_from_u8 (envelope->attack));
  emu_print (1, 5, "Hold:    %.02f s\n",
	     emu3_get_time_163_69_from_u8 (envelope->hold));
  emu_print (1, 5, "Decay:   %.02f s\n",
	     emu3_get_time_163_69_from_u8 (envelope->decay));
  emu_print (1, 5, "Sustain:  %3d %%\n",
	     emu3_get_percent_from_s8 (envelope->sustain));
  emu_print (1, 5, "Release: %.02f s\n",
	     emu3_get_time_163_69_from_u8 (envelope->release));
}

static gint
emu3_get_sample_num (struct emu3_preset_zone *zone)
{
  return (zone->sample_id_msb << 8) + zone->sample_id_lsb;
}

static void
emu3_print_preset_zone_info (struct emu_file *file,
			     struct emu3_preset_zone *zone)
{
  emu_print (1, 3, "Dynamic Setup\n");
  emu_print (1, 4, "Tuning: %.1f cents\n",
	     emu3_get_note_tuning_from_s8 (zone->note_tuning));
  emu_print (1, 4, "Delay:    %.3f s\n",
	     emu3_get_note_on_delay (zone->note_on_delay));
  emu_print (1, 4, "Chorus:       %s\n",
	     emu3_get_is_chorus_enabled (zone->flags) ? "on" : "off");
  emu_print (1, 4, "Original Key: %3s\n",
	     emu_get_note_name (zone->original_key));
  emu_print (1, 4, "Sample:       %03d\n", emu3_get_sample_num (zone));
  emu_print (1, 4, "Disable Loop: %s\n",
	     emu3_get_is_loop_disabled (zone->flags) ? "on" : "off");
  emu_print (1, 4, "Disable Side: %s\n",
	     SIDES_DISABLED[1 + emu3_get_is_side_disabled (zone->flags)]);

  emu_print (1, 3, "VCA\n");
  emu_print (1, 4, "Level: %d %%\n",
	     emu3_get_percent_from_s8 (zone->vca_level));
  emu_print (1, 4, "Pan:   %d %%\n",
	     emu3_get_percent_signed_from_s8 (zone->vca_pan));
  emu3_print_envelope (&zone->vca_envelope);

  emu_print (1, 3, "VCF\n");
  //Filter type might only work with ESI banks
  gint vcf_type = zone->vcf_type_lfo_shape >> 3;
  if (vcf_type > VCF_TYPE_SIZE - 1)
    vcf_type = VCF_TYPE_SIZE - 1;
  emu_print (1, 4, "Type: %16s\n", VCF_TYPE[vcf_type]);
  //Cutoff: [0, 255] -> [26, 74040]
  gint cutoff = zone->vcf_cutoff;
  emu_print (1, 4, "Cutoff:       %d Hz\n",
	     emu3_get_vcf_cutoff_frequency_from_u8 (cutoff));
  // Filter Q might only work with ESI banks.
  // ESI Q: [0x80, 0xff] -> [0, 100]
  // Other formats: [0, 0x7f]
  // The most significat bit is a flag to enable real time control VCof F NoteOn Q.
  // It can be disabled on the unit via Dynamic Processing / Realtime Enable.
  emu_print (1, 4, "Q:               % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->vcf_q & 0x7f));
  emu_print (1, 4, "Tracking:         %.2f\n",
	     emu3_get_vcf_tracking_from_s8 (zone->vcf_tracking));
  emu_print (1, 4, "Envelope Amount: % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->vcf_envelope_amount));
  emu3_print_envelope (&zone->vcf_envelope);

  emu_print (1, 3, "LFO\n");
  emu_print (1, 4, "Rate:      %.2f Hz\n",
	     emu3_get_lfo_rate (zone->lfo_rate));
  emu_print (1, 4, "Shape:    %s\n",
	     LFO_SHAPE[zone->vcf_type_lfo_shape & 0x3]);
  emu_print (1, 4, "Delay:      %.2f s\n",
	     emu3_get_time_21_69 (zone->lfo_delay));
  emu_print (1, 4, "Variation:   % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->lfo_variation));
  emu_print (1, 4, "LFO->Pitch:  % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->lfo_to_pitch));
  emu_print (1, 4, "LFO->Cutoff: % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->lfo_to_cutoff));
  emu_print (1, 4, "LFO->VCA:    % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->lfo_to_vca));
  emu_print (1, 4, "LFO->Pan:    % 3d %%\n",
	     emu3_get_percent_from_s8 (zone->lfo_to_pan));

  emu_print (1, 3, "Auxiliary envelope\n");
  emu_print (1, 4, "Destination:        %-3s\n",
	     AUX_ENV_DST[zone->aux_envelope_dest]);
  emu_print (1, 4, "Envelope amount: % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->aux_envelope_amount));
  emu3_print_envelope (&zone->aux_envelope);

  emu_print (1, 3, "Velocity to\n");
  emu_print (1, 4, "Pitch:              % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_pitch));
  emu_print (1, 4, "VCA Level:          % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_vca_level));
  emu_print (1, 4, "VCA Attack:         % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_vca_attack));
  emu_print (1, 4, "VCF Cutoff:         % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_vcf_cutoff));
  emu_print (1, 4, "VCF Q:              % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_vcf_q));
  emu_print (1, 4, "VCF Attack:         % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_vcf_attack));
  emu_print (1, 4, "Pan:                % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_pan));
  emu_print (1, 4, "Sample Start:       % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_sample_start));
  emu_print (1, 4, "Auxiliary Envelope: % 4d %%\n",
	     emu3_get_percent_from_s8 (zone->vel_to_aux_env));

  emu_print (1, 3, "Keyboard Mode\n");
  emu_print (1, 4, "Keyboard Envelope mode: %s\n",
	     emu3_get_env_mode_trigger (zone->flags) ? "trigger" : "gate");
  emu_print (1, 4, "Solo:                   %s\n",
	     emu3_get_is_solo_enabled (zone->flags) ? "on" : "off");
  emu_print (1, 4, "Nontranspose:           %s\n",
	     emu3_get_is_nontranspose_enabled (zone->flags) ? "on" : "off");

  emu_print (1, 3, "Realtime Enable\n");
  emu_print (1, 4, "Pitch:           %s\n",
	     OFF_ON[emu3_get_is_rt_pitch_enabled (zone->rt_enable_flags)]);
  emu_print (1, 4, "VCF cutoff:      %s\n",
	     OFF_ON[emu3_get_is_rt_vcf_cutoff_enabled
		    (zone->rt_enable_flags)]);
  emu_print (1, 4, "VCF NoteOn Q:    %s\n",
	     OFF_ON[emu3_get_is_rt_note_on_q_enabled (zone->vcf_q)]);
  emu_print (1, 4, "LFO->Pitch:      %s\n",
	     OFF_ON[emu3_get_is_rt_lfo_pitch_enabled
		    (zone->rt_enable_flags)]);
  emu_print (1, 4, "LFO->VCF cutoff: %s\n",
	     OFF_ON[emu3_get_is_rt_lfo_vcf_cutoff_enabled
		    (zone->rt_enable_flags)]);
  emu_print (1, 4, "LFO->VCA:        %s\n",
	     OFF_ON[emu3_get_is_rt_lfo_vca_enabled (zone->rt_enable_flags)]);
  emu_print (1, 4, "VCA level:       %s\n",
	     OFF_ON[emu3_get_is_rt_vca_level_enabled
		    (zone->rt_enable_flags)]);
  emu_print (1, 4, "Attack:          %s\n",
	     OFF_ON[emu3_get_is_rt_attack_enabled (zone->rt_enable_flags)]);
  emu_print (1, 4, "Pan:             %s\n",
	     OFF_ON[emu3_get_is_rt_pan_enabled (zone->rt_enable_flags)]);
}

static void
emu3_print_preset_info (struct emu3_preset *preset)
{
  for (gint i = 0; i < RT_CONTROLS_SRC_SIZE; i++)
    {
      gint dst = 0;
      for (gint j = 0; j < RT_CONTROLS_SIZE; j++)
	{
	  if (preset->rt_controls[j] == i + 1)
	    {
	      dst = j + 1;
	      break;
	    }
	}
      emu_print (1, 1, "Mapping: %s - %s\n", RT_CONTROLS_SRC[i],
		 RT_CONTROLS_DST[dst]);
    }
  for (gint i = 0; i < RT_CONTROLS_FS_SIZE; i++)
    {
      emu_print (1, 1, "Mapping: %s - %s\n",
		 RT_CONTROLS_FS_SRC[i],
		 RT_CONTROLS_FS_DST[preset->rt_controls
				    [RT_CONTROLS_SIZE + i]]);
    }
  for (gint i = 0; i < PRESET_UNKNOWN_0_SIZE; i++)
    {
      emu_print (2, 1, "Unknown_0 %d: %d\n", i, preset->unknown_0[i]);
    }

  emu_print (1, 1, "Pitch Bend Range: %d\n", preset->pitch_bend_range);

  emu_print (1, 1, "Velocity Ranges:\n");
  emu_print (1, 2, "Pri: %d to %d\n", preset->velocity_range_pri_low,
	     preset->velocity_range_pri_high);
  emu_print (1, 2, "Sec: %d to %d\n", preset->velocity_range_sec_low,
	     preset->velocity_range_sec_high);
  guint16 p = preset->link_preset_lsb | (preset->link_preset_msb << 8);
  if (p == 0)
    {
      emu_print (1, 1, "Link Preset to: Off\n");
    }
  else
    {
      emu_print (1, 1, "Link Preset to: %03d\n", p - 1);
    }

  for (gint i = 0; i < PRESET_UNKNOWN_1_SIZE; i++)
    {
      emu_print (2, 1, "Unknown_1 %d: %d\n", i, preset->unknown_1[i]);
    }
}

static void
emu3_set_preset_rt_control (struct emu3_preset *preset, gint src, gint dst)
{
  if (dst >= 0 && dst < RT_CONTROLS_DST_SIZE)
    {
      emu_debug (1, "Setting controller %s to %s...",
		 RT_CONTROLS_SRC[src], RT_CONTROLS_DST[dst]);
      if (dst >= 0)
	{
	  for (gint i = 0; i < RT_CONTROLS_SIZE; i++)
	    if (preset->rt_controls[i] == src + 1)
	      preset->rt_controls[i] = 0;
	  if (dst > 0)
	    preset->rt_controls[dst - 1] = src + 1;
	}
    }
  else
    emu_error ("Invalid destination %d for %s", dst, RT_CONTROLS_SRC[src]);
}

static void
emu3_set_preset_rt_control_fs (struct emu3_preset *preset, gint src, gint dst)
{
  if (dst >= 0 && dst < RT_CONTROLS_FS_DST_SIZE)
    {
      emu_debug (1, "Setting controller %s to %s...",
		 RT_CONTROLS_FS_SRC[src], RT_CONTROLS_FS_DST[dst]);
      preset->rt_controls[src + RT_CONTROLS_FS_DST_SIZE] = dst;
    }
  else
    emu_error ("Invalid destination %d for %s", dst, RT_CONTROLS_FS_SRC[src]);
}

static void
emu3_set_preset_zone_level (struct emu3_preset_zone *zone, gint level)
{
  if (level < 0 || level > 100)
    {
      emu_error ("Value %d for level not in range [0, 100]", level);
    }
  else
    {
      emu_debug (1, "Setting level to %d...", level);
      zone->vca_level = (guint8) (level * 127 / 100);
    }
}

static void
emu3_set_preset_zone_cutoff (struct emu3_preset_zone *zone, gint cutoff)
{
  if (cutoff < 0 || cutoff > 255)
    {
      emu_error ("Value for cutoff %d not in range [0, 255]", cutoff);
    }
  else
    {
      emu_debug (1, "Setting cutoff to %d...", cutoff);
      zone->vcf_cutoff = (guint8) cutoff;
    }
}

static void
emu3_set_preset_zone_q (struct emu_file *file, struct emu3_preset_zone *zone,
			gint q)
{
  if (q < 0 || q > 100)
    {
      emu_error ("Value %d for Q not in range [0, 100]", q);
    }
  else
    {
      struct emu3_bank *bank = EMU3_BANK (file);
      emu_debug (1, "Setting Q to %d...", q);
      zone->vcf_q = emu3_get_s8_from_percent (q);

      // HVG
      // If this is configured, we might want to assume
      // that the user wants this enabled.
      // The bit 7 / 0x80 controls this setting on ESI32.
      // I suspect other units might ignore it, so it could
      // always be enabled perhaps?
      // zone->vcf_q |= 0x80

      if (strcmp (ESI_32_V3_DEF, bank->format) == 0)
	{
	  zone->vcf_q |= 0x80;
	}
    }
}

static void
emu3_set_preset_zone_filter (struct emu3_preset_zone *zone, gint filter)
{
  if (filter < 0 || filter > VCF_TYPE_SIZE - 2)
    {
      emu_error ("Value %d for filter not in range [0, %d]",
		 filter, VCF_TYPE_SIZE - 2);
    }
  else
    {
      emu_debug (1, "Setting filter to '%s'...", VCF_TYPE[filter]);
      zone->vcf_type_lfo_shape =
	(((guint8) filter) << 3) | (zone->vcf_type_lfo_shape & 0x3);
    }
}

static void
emu3_set_preset_rt_controls (struct emu3_preset *preset, gchar *rt_controls)
{
  gchar *token;
  gchar *endtoken;
  gint i;
  gint controller;

  if (rt_controls == NULL)
    return;

  emu_debug (1, "Setting realtime controls...");
  i = 0;
  while (i < RT_CONTROLS_SIZE && (token = strsep (&rt_controls, ",")) != NULL)
    {
      if (*token == '\0')
	{
	  emu_error ("Empty value");
	}
      else
	{
	  controller = strtol (token, &endtoken, 10);
	  if (*endtoken == '\0')
	    {
	      if (i < RT_CONTROLS_SRC_SIZE)
		emu3_set_preset_rt_control (preset, i, controller);
	      else if (i < RT_CONTROLS_SRC_SIZE + RT_CONTROLS_FS_SRC_SIZE)
		{
		  emu3_set_preset_rt_control_fs (preset,
						 i - RT_CONTROLS_SRC_SIZE,
						 controller);
		}
	      else
		emu_error ("Too many controls");
	    }
	  else
	    emu_error ("'%s' not a number", token);
	}
      i++;
    }
}

static void
emu3_set_preset_pbr (struct emu3_preset *preset, gint pbr)
{
  if (pbr < 0 || pbr > 36)
    {
      emu_error ("Value for pitch bend range %d not in range [0, 36]", pbr);
    }
  else
    {
      emu_debug (1, "Setting pitch bend range to %d...", pbr);
      preset->pitch_bend_range = pbr;
    }
}

static guint32 *
emu3_get_preset_addresses (struct emu3_bank *bank)
{
  gchar *raw = (gchar *) bank;

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return (guint32 *) & raw[PRESET_SIZE_ADDR_START_EMU_3X];
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return (guint32 *) & raw[PRESET_SIZE_ADDR_START_EMU_THREE];
  else
    return NULL;
}

static guint32
emu3_get_preset_address (struct emu3_bank *bank, gint preset)
{
  guint32 offset = emu3_get_preset_addresses (bank)[preset];

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return PRESET_START_EMU_3X + offset;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return PRESET_START_EMU_THREE + offset - PRESET_OFFSET_EMU_THREE;
  else
    return 0;
}

static gint
emu3_get_max_presets (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return MAX_PRESETS_EMU_3X;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return MAX_PRESETS_EMU_THREE;
  else
    return 0;
}

static guint32
emu3_get_sample_start_address (struct emu3_bank *bank)
{
  gint max_presets = emu3_get_max_presets (bank);
  guint32 *paddresses = emu3_get_preset_addresses (bank);
  guint32 offset = paddresses[max_presets];
  guint32 start_address;

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    //There is always a 0xee byte before the samples
    start_address = PRESET_START_EMU_3X + 1;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    //There is always a 0x00 byte before the samples
    start_address = PRESET_START_EMU_THREE + 1 - PRESET_OFFSET_EMU_THREE;
  else
    start_address = 0;

  return start_address + offset;
}

static guint32 *
emu3_get_sample_addresses (struct emu3_bank *bank)
{
  gchar *raw = (gchar *) bank;

  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return (guint32 *) & (raw[SAMPLE_ADDR_START_EMU_3X]);
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return (guint32 *) & (raw[SAMPLE_ADDR_START_EMU_THREE]);
  else
    return NULL;
}

static gint
emu3_get_max_samples (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0)
    return MAX_SAMPLES_EMU_3X;
  else if (strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return MAX_SAMPLES_EMU_THREE;
  else
    return 0;
}

static guint32
emu3_get_next_sample_address (struct emu3_bank *bank)
{
  gint max_samples = emu3_get_max_samples (bank);
  guint32 *saddresses = emu3_get_sample_addresses (bank);
  guint32 sample_start_addr = emu3_get_sample_start_address (bank);

  return sample_start_addr + saddresses[max_samples] - SAMPLE_OFFSET;
}

static gint
emu3_check_bank_format (struct emu3_bank *bank)
{
  if (strcmp (EMULATOR_3X_DEF, bank->format) == 0 ||
      strcmp (ESI_32_V3_DEF, bank->format) == 0 ||
      strcmp (EMULATOR_THREE_DEF, bank->format) == 0)
    return 1;
  else
    return 0;
}

struct emu3_preset *
emu3_get_preset (struct emu_file *file, gint preset_num)
{
  struct emu3_bank *bank = EMU3_BANK (file);
  guint32 addr = emu3_get_preset_address (bank, preset_num);
  return (struct emu3_preset *) &file->raw[addr];
}

static guint32
emu3_get_preset_note_zone_addr (struct emu_file *file, gint preset_num)
{
  struct emu3_bank *bank = EMU3_BANK (file);
  guint32 addr = emu3_get_preset_address (bank, preset_num);
  return addr + sizeof (struct emu3_preset);
}

static guint32
emu3_get_preset_zone_addr (struct emu_file *file, gint preset_num)
{
  struct emu3_preset *preset = emu3_get_preset (file, preset_num);
  return emu3_get_preset_note_zone_addr (file, preset_num) +
    (preset->note_zones) * sizeof (struct emu3_preset_note_zone);
}

struct emu3_preset_note_zone *
emu3_get_preset_note_zones (struct emu_file *file, gint preset_num)
{
  guint32 addr = emu3_get_preset_note_zone_addr (file, preset_num);
  return (struct emu3_preset_note_zone *) &file->raw[addr];
}

struct emu3_preset_zone *
emu3_get_preset_zones (struct emu_file *file, gint preset_num)
{
  guint32 addr = emu3_get_preset_zone_addr (file, preset_num);
  return (struct emu3_preset_zone *) &file->raw[addr];
}

static gint
emu3_count_preset_zones (struct emu_file *file, gint preset_num)
{
  gint i, max = -1;
  struct emu3_preset *preset = emu3_get_preset (file, preset_num);
  struct emu3_preset_note_zone *note_zone =
    emu3_get_preset_note_zones (file, preset_num);

  for (i = 0; i < preset->note_zones; i++)
    {
      if (note_zone->pri_zone != 0xff && note_zone->pri_zone > max)
	max = note_zone->pri_zone;
      if (note_zone->sec_zone != 0xff && note_zone->sec_zone > max)
	max = note_zone->sec_zone;
      note_zone++;
    }
  return max + 1;
}

struct emu_file *
emu3_open_file (const gchar *name)
{
  struct emu3_bank *bank;
  struct emu_file *file = emu_open_file (name);

  if (!file)
    {
      return NULL;
    }

  bank = EMU3_BANK (file);

  if (!emu3_check_bank_format (bank))
    {
      emu_error ("Bank format not supported");
      emu_close_file (file);
      return NULL;
    }

  emu_print (1, 0, "Bank name: %.*s\n", EMU3_NAME_SIZE, bank->name);
  emu_print (1, 0, "Bank size: %zuB\n", file->size);
  emu_print (1, 0, "Bank format: %s\n", bank->format);

  emu_print (2, 1, "Geometry:\n");
  emu_print (2, 1, "Objects: %d\n", bank->objects + 1);
  emu_print (2, 1, "Next sample: 0x%08x\n", bank->next_sample);

  if (bank->preset_blocks + bank->sample_blocks != bank->total_blocks)
    {
      emu_error ("Block sum error\n");
    }

  if (strncmp (bank->name, bank->name_copy, EMU3_NAME_SIZE))
    emu_print (2, 1, "Bank name is different than previously found.\n");

  emu_print (2, 1, "Selected preset: %d\n", bank->selected_preset);

  emu_print (2, 1, "Preset blocks: %d\n", bank->preset_blocks);
  emu_print (2, 1, "Sample blocks: %d\n", bank->sample_blocks);
  emu_print (2, 1, "Total  blocks: %d\n", bank->total_blocks);

  emu_print (2, 1, "Parameters:\n");
  for (gint i = 0; i < BANK_PARAMETERS; i++)
    emu_print (2, 1, "Parameter %d: 0x%08x (%d)\n", i,
	       bank->parameters[i], bank->parameters[i]);

  emu_print (2, 1, "Current preset: %d\n", bank->parameters[0]);
  emu_print (2, 1, "Current sample: %d\n", bank->parameters[1]);

  return file;
}

static void
emu3_process_zone (struct emu_file *file, struct emu3_preset_zone *zone,
		   gint level, gint cutoff, gint q, gint filter)
{
  if (level != -1)
    emu3_set_preset_zone_level (zone, level);
  if (cutoff != -1)
    emu3_set_preset_zone_cutoff (zone, cutoff);
  if (q != -1)
    emu3_set_preset_zone_q (file, zone, q);
  if (filter != -1)
    emu3_set_preset_zone_filter (zone, filter);
}

static void
emu3_process_note_zone (struct emu_file *file,
			struct emu3_preset_zone *zones,
			struct emu3_preset_note_zone *note_zone, gint level,
			gint cutoff, gint q, gint filter)
{
  emu_print (1, 2, "options: %02x %02x\n", note_zone->options_lsb,
	     note_zone->options_msb);

  //If the zone is for pri, sec layer or both
  if (note_zone->pri_zone != 0xff)
    {
      emu_print (1, 2, "pri\n");
      struct emu3_preset_zone *zone = &zones[note_zone->pri_zone];
      emu3_process_zone (file, zone, level, cutoff, q, filter);
      emu3_print_preset_zone_info (file, zone);
    }
  if (note_zone->sec_zone != 0xff)
    {
      emu_print (1, 2, "sec\n");
      struct emu3_preset_zone *zone = &zones[note_zone->sec_zone];
      emu3_process_zone (file, zone, level, cutoff, q, filter);
      emu3_print_preset_zone_info (file, zone);
    }
}

static void
emu3_process_preset (struct emu_file *file, gint preset_num,
		     gchar *rt_controls, gint pbr, gint level, gint cutoff,
		     gint q, gint filter)
{
  struct emu3_preset *preset = emu3_get_preset (file, preset_num);
  struct emu3_preset_zone *zones;
  struct emu3_preset_note_zone *note_zones;

  emu_print (0, 0, "Preset %03d: %.*s\n", preset_num, EMU3_NAME_SIZE,
	     preset->name);

  if (rt_controls)
    {
      gchar *rtc = strdup (rt_controls);
      emu3_set_preset_rt_controls (preset, rtc);
      free (rtc);
    }

  if (pbr != -1)
    {
      emu3_set_preset_pbr (preset, pbr);
    }

  emu3_print_preset_info (preset);

  emu_print (1, 1, "Note mappings:\n");
  for (gint j = 0; j < EMU3_NOTES; j++)
    {
      if (preset->note_zone_mappings[j] != 0xff)
	emu_print (1, 2, "%-4s: %2d\n", emu_get_note_name (j),
		   preset->note_zone_mappings[j]);
    }

  note_zones = emu3_get_preset_note_zones (file, preset_num);
  zones = emu3_get_preset_zones (file, preset_num);

  emu_print (1, 1, "Zones: %d\n", preset->note_zones);
  for (gint j = 0; j < preset->note_zones; j++)
    {
      emu_print (1, 1, "Zone %d\n", j);
      emu3_process_note_zone (file, zones, note_zones, level, cutoff, q,
			      filter);
      note_zones++;
    }
}

static gint
emu3_find_first_zone_for_sample (struct emu_file *file, gint sample_num,
				 struct emu3_preset_zone **first_zone)
{
  guint32 *addresses;
  struct emu3_bank *bank = EMU3_BANK (file);
  gint preset_num, max_presets = emu3_get_max_presets (bank);
  struct emu3_preset *preset;
  struct emu3_preset_note_zone *note_zones;
  struct emu3_preset_zone *zones;
  struct emu3_preset_zone *zone;

  preset_num = 0;
  addresses = emu3_get_preset_addresses (bank);
  while (preset_num < max_presets)
    {
      if (addresses[0] != addresses[1])
	{
	  preset = emu3_get_preset (file, preset_num);
	  note_zones = emu3_get_preset_note_zones (file, preset_num);
	  zones = emu3_get_preset_zones (file, preset_num);
	  for (gint i = 0; i < preset->note_zones; i++)
	    {
	      zone = NULL;
	      if (note_zones->pri_zone != 0xff)
		{
		  zone = &zones[note_zones->pri_zone];
		}
	      if (note_zones->sec_zone != 0xff)
		{
		  zone = &zones[note_zones->sec_zone];
		}

	      if (zone != NULL && emu3_get_sample_num (zone) == sample_num)
		{
		  *first_zone = zone;
		  return 1;
		}

	      note_zones++;
	    }
	}
      addresses++;
      preset_num++;
    }

  return 0;
}

static gint
emu3_get_sample (struct emu_file *file, gint sample_num,
		 struct emu3_sample **sample)
{
  struct emu3_bank *bank = EMU3_BANK (file);
  gint max_samples = emu3_get_max_samples (bank);

  if (sample_num - 1 < max_samples)
    {
      guint32 *addresses = emu3_get_sample_addresses (bank);
      guint32 sample_start_addr = emu3_get_sample_start_address (bank);
      guint32 address = sample_start_addr + addresses[sample_num - 1] -
	SAMPLE_OFFSET;
      *sample = (struct emu3_sample *) &file->raw[address];
      return 0;
    }
  else
    {
      emu_error ("Sample %03d not available", sample_num);
      *sample = NULL;
      return -1;
    }
}

gint
emu3_process_bank (struct emu_file *file, gint ext_mode, gint edit_preset,
		   gchar *rt_controls, gint pbr, gint level, gint cutoff,
		   gint q, gint filter)
{
  gint i;
  guint32 *addresses;
  guint32 address;
  guint32 sample_start_addr;
  guint32 next_sample_addr;
  struct emu3_sample *sample;
  gint max_samples;
  struct emu3_bank *bank = EMU3_BANK (file);
  gint max_presets = emu3_get_max_presets (bank);

  i = 0;
  addresses = emu3_get_preset_addresses (bank);
  while (i < max_presets)
    {
      if (addresses[0] != addresses[1])
	{
	  emu3_process_preset (file, i, rt_controls, pbr, level, cutoff, q,
			       filter);
	}
      addresses++;
      i++;
    }

  sample_start_addr = emu3_get_sample_start_address (bank);
  emu_print (1, 0, "Sample start: 0x%08x\n", sample_start_addr);

  max_samples = emu3_get_max_samples (bank);
  addresses = emu3_get_sample_addresses (bank);
  emu_print (1, 0, "Start with offset: 0x%08x\n", addresses[0]);
  emu_print (1, 0, "Next with offset: 0x%08x\n", addresses[max_samples]);
  next_sample_addr = emu3_get_next_sample_address (bank);
  emu_print (1, 0, "Next sample: 0x%08x (equals bank size)\n",
	     next_sample_addr);

  i = 0;
  while (addresses[i] != 0 && i < max_samples)
    {
      struct emu3_preset_zone *zone;
      guint32 original_key = 0;
      gfloat fraction = 0;
      address = sample_start_addr + addresses[i] - SAMPLE_OFFSET;
      sample = (struct emu3_sample *) &file->raw[address];
      if (ext_mode)
	{
	  if (emu3_find_first_zone_for_sample (file, i + 1, &zone))
	    {
	      original_key = zone->original_key;
	      fraction = emu3_get_note_tuning_from_s8 (zone->note_tuning);
	    }
	}
      emu3_process_sample (sample, i + 1, ext_mode, original_key, fraction);
      i++;
    }

  return EXIT_SUCCESS;
}

static gint
emu3_get_bank_presets (struct emu3_bank *bank)
{
  guint32 *paddresses = emu3_get_preset_addresses (bank);
  gint max_presets = emu3_get_max_presets (bank);
  gint total = 0;

  while (paddresses[0] != paddresses[1] && total < max_presets)
    {
      paddresses++;
      total++;
    }

  return total;
}

static gint
emu3_get_bank_samples (struct emu3_bank *bank)
{
  guint32 *saddresses = emu3_get_sample_addresses (bank);
  gint max_samples = emu3_get_max_samples (bank);
  gint total = 0;

  while (saddresses[0] != 0 && total < max_samples)
    {
      saddresses++;
      total++;
    }

  return total;
}

gint
emu3_add_sample (struct emu_file *file, gchar *sample_path, gint *sample_num,
		 gboolean *mono_out, guint32 *frames_out)
{
  gboolean mono;
  guint32 frames;
  gint size, next_sample, sample_offset;
  struct emu3_bank *bank = EMU3_BANK (file);
  gint max_samples = emu3_get_max_samples (bank);
  gint total_samples = emu3_get_bank_samples (bank);
  guint32 *saddresses = emu3_get_sample_addresses (bank);
  guint32 sample_start_addr = emu3_get_sample_start_address (bank);
  guint32 next_sample_addr = emu3_get_next_sample_address (bank);
  struct emu3_sample *sample =
    (struct emu3_sample *) &file->raw[next_sample_addr];

  if (total_samples == max_samples)
    {
      emu_error ("Sample limit reached");
      return EXIT_FAILURE;
    }

  next_sample = total_samples + 1;	//Sample number is 1 based
  if (sample_num)
    {
      *sample_num = next_sample;
    }

  emu_debug (1, "Adding sample %d...", next_sample);
  sample_offset = next_sample_addr - sample_start_addr - total_samples * 2;
  size = emu3_append_sample (file, sample, sample_path, sample_offset,
			     &mono, &frames);
  if (size < 0)
    {
      emu_error ("Appending sample error");
      return -size;
    }

  if (mono_out)
    {
      *mono_out = mono;
    }

  if (frames_out)
    {
      *frames_out = frames;
    }

  file->size += size;

  bank->objects++;
  bank->next_sample = next_sample_addr + size - sample_start_addr;
  saddresses[total_samples] = saddresses[max_samples];
  saddresses[max_samples] = bank->next_sample + SAMPLE_OFFSET;

  return EXIT_SUCCESS;
}

static void
emu3_reset_envelope (struct emu3_envelope *envelope)
{
  envelope->attack = 0;
  envelope->hold = 0;
  envelope->decay = 0;
  envelope->sustain = 0x7f;
  envelope->release = 0;
}

static gint
emu3_add_zones (struct emu_file *file, gint preset_num, gint zone_num,
		struct emu3_preset_zone **preset_zone)
{
  void *src, *dst, *zone_src, *zone_dst;
  guint32 next_preset_addr, dst_addr;
  guint32 *paddresses;
  gsize size, inc_size;
  guint32 next_sample_addr;
  struct emu3_preset *preset;
  struct emu3_preset_note_zone *note_zone;
  gint max_presets;
  struct emu3_bank *bank;

  inc_size = sizeof (struct emu3_preset_zone);
  if (zone_num == -1)
    inc_size += sizeof (struct emu3_preset_note_zone);

  if (file->size + inc_size > EMU3_MEM_SIZE)
    return -1;

  bank = EMU3_BANK (file);
  next_preset_addr = emu3_get_preset_address (bank, preset_num + 1);
  dst_addr = next_preset_addr + inc_size;

  paddresses = emu3_get_preset_addresses (bank);
  max_presets = emu3_get_max_presets (bank);
  for (gint i = preset_num + 1; i < max_presets + 1; i++)
    paddresses[i] += inc_size;

  next_sample_addr = emu3_get_next_sample_address (bank);
  size = next_sample_addr - next_preset_addr;

  emu_debug (3, "Moving %zu B from 0x%08x to 0x%08x...", size,
	     next_preset_addr, dst_addr);

  src = &file->raw[next_preset_addr];
  dst = &file->raw[dst_addr];
  memmove (dst, src, size);

  preset = emu3_get_preset (file, preset_num);

  if (zone_num == -1)
    {
      guint32 zone_addr = emu3_get_preset_zone_addr (file, preset_num);
      guint32 zone_addr_dst =
	zone_addr + sizeof (struct emu3_preset_note_zone);

      zone_src = &file->raw[zone_addr];
      zone_dst = &file->raw[zone_addr_dst];

      if (preset->note_zones > 0)
	{
	  size = next_preset_addr - zone_addr;

	  emu_debug (3, "Moving %zu B from from 0x%08x to 0x%08x...", size,
		     zone_addr, zone_addr_dst);

	  memmove (zone_dst, zone_src, size);
	}

      note_zone = zone_src;
      note_zone->options_lsb = 0;
      note_zone->options_msb = 0;
      note_zone->pri_zone = preset->note_zones;
      note_zone->sec_zone = 0xff;

      guint32 next_zone_addr =
	next_preset_addr + sizeof (struct emu3_preset_note_zone);

      *preset_zone = (struct emu3_preset_zone *) &file->raw[next_zone_addr];

      preset->note_zones++;
    }
  else
    {
      note_zone = &emu3_get_preset_note_zones (file, preset_num)[zone_num];
      note_zone->sec_zone = emu3_count_preset_zones (file, preset_num);

      *preset_zone = (struct emu3_preset_zone *) &file->raw[next_preset_addr];
    }

  return inc_size;
}

gint
emu3_add_preset_zone (struct emu_file *file, gint preset_num, gint sample_num,
		      struct emu_zone_range *zone_range,
		      struct emu3_preset_zone **zone_)
{
  gint sec_zone_id;
  struct emu3_bank *bank = EMU3_BANK (file);
  gint total_presets = emu3_get_bank_presets (bank);
  guint32 inc_size = 0;
  struct emu3_preset *preset;
  struct emu3_preset_zone *zone;

  if (preset_num < 0 || preset_num >= total_presets)
    {
      emu_error ("Invalid preset number: %d", preset_num);
      return EXIT_FAILURE;
    }

  emu_debug (2,
	     "Adding sample %d to layer %d with original key %s (%d) from %s (%d) to %s (%d) to preset %d...",
	     sample_num, zone_range->layer,
	     emu_get_note_name (zone_range->original_key),
	     zone_range->original_key,
	     emu_get_note_name (zone_range->lower_key),
	     zone_range->lower_key,
	     emu_get_note_name (zone_range->higher_key),
	     zone_range->higher_key, preset_num);

  preset = emu3_get_preset (file, preset_num);

  if (zone_range->layer == 1)
    {
      gint assigned = 0;
      for (gint i = zone_range->lower_key; i <= zone_range->higher_key; i++)
	{
	  if (preset->note_zone_mappings[i] != 0xff)
	    {
	      assigned = 1;
	    }
	}
      if (assigned == 1)
	{
	  emu_error ("Zone already assigned to notes");
	  return EXIT_FAILURE;
	}

      for (gint i = zone_range->lower_key; i <= zone_range->higher_key; i++)
	{
	  preset->note_zone_mappings[i] = preset->note_zones;
	}

      inc_size = emu3_add_zones (file, preset_num, -1, &zone);
      if (inc_size < 0)
	{
	  emu_error ("No space left in bank");
	  return EXIT_FAILURE;
	}
    }
  else if (zone_range->layer == 2)
    {
      sec_zone_id = preset->note_zone_mappings[zone_range->lower_key];
      gint several = 0;
      for (gint i = zone_range->lower_key + 1; i <= zone_range->higher_key;
	   i++)
	{
	  if (preset->note_zone_mappings[i] != sec_zone_id)
	    {
	      several = 1;
	    }
	}
      if (several == 1)
	{
	  emu_error ("Note range in several zones");
	  return EXIT_FAILURE;
	}

      inc_size = emu3_add_zones (file, preset_num, sec_zone_id, &zone);
      if (inc_size < 0)
	{
	  emu_error ("No space left for zones");
	  return EXIT_FAILURE;
	}
    }

  if (zone_)
    {
      *zone_ = zone;
    }

  zone->original_key = zone_range->original_key;
  zone->sample_id_lsb = sample_num % 256;
  zone->sample_id_msb = sample_num / 256;
  zone->parameter_a = 0x1f;
  emu3_reset_envelope (&zone->vca_envelope);
  zone->lfo_rate = 0x41;
  zone->lfo_delay = 0x00;
  zone->lfo_variation = 0;
  zone->vcf_cutoff = DEFAULT_CUTOFF_U8;
  zone->vcf_q = strcmp (ESI_32_V3_DEF, bank->format) == 0 ? 0x80 : 0;
  zone->vcf_envelope_amount = 0;
  emu3_reset_envelope (&zone->vcf_envelope);
  emu3_reset_envelope (&zone->aux_envelope);
  zone->aux_envelope_amount = 0;
  zone->aux_envelope_dest = 0;
  zone->vel_to_vca_level = 0;
  zone->vel_to_vca_attack = 0;
  zone->vel_to_vcf_cutoff = 0;
  zone->vel_to_pitch = 0;
  zone->vel_to_aux_env = 0;
  zone->vel_to_vcf_q = 0;
  zone->vel_to_vcf_attack = 0;
  zone->vel_to_sample_start = 0;
  zone->vel_to_pan = 0;
  zone->lfo_to_pitch = 0;
  zone->lfo_to_vca = 0;
  zone->lfo_to_cutoff = 0;
  zone->lfo_to_pan = 0;
  zone->vca_level = 0x7f;
  zone->note_tuning = 0;
  zone->vcf_tracking = 0x40;
  zone->note_on_delay = 0;
  zone->vca_pan = 0x40;
  zone->vcf_type_lfo_shape = 0x8;
  zone->rt_enable_flags = 0xff;
  zone->flags = 0x01;

  bank->next_preset += inc_size;
  file->size += inc_size;

  return EXIT_SUCCESS;
}

gint
emu3_del_preset_zone (struct emu_file *file, gint preset_num, gint zone_num)
{
  struct emu3_bank *bank = EMU3_BANK (file);
  gint zones, max_presets, total_presets = emu3_get_bank_presets (bank);
  guint32 *paddresses, src_addr, dst_addr, dec_size_note_zone,
    dec_size_zone, size, addr;
  void *src, *dst;
  struct emu3_preset *preset;
  struct emu3_preset_note_zone *note_zone;

  if (preset_num < 0 || preset_num >= total_presets)
    {
      emu_error ("Invalid preset number: %d", preset_num);
      return EXIT_FAILURE;
    }

  zones = emu3_count_preset_zones (file, preset_num);

  if (zone_num < 0 || zone_num >= zones)
    {
      emu_error ("Invalid zone number: %d", zone_num);
      return EXIT_FAILURE;
    }

  preset = emu3_get_preset (file, preset_num);

  note_zone = emu3_get_preset_note_zones (file, preset_num);
  note_zone += zone_num;

  addr = emu3_get_preset_note_zone_addr (file, preset_num);
  dec_size_note_zone = sizeof (struct emu3_preset_note_zone);
  src_addr = addr + sizeof (struct emu3_preset_note_zone) * (zone_num + 1);
  dst_addr = addr + sizeof (struct emu3_preset_note_zone) * zone_num;
  src = &file->raw[src_addr];
  dst = &file->raw[dst_addr];
  size = file->size - src_addr;
  emu_debug (3, "Moving %d B from 0x%08x to 0x%08x...", size,
	     src_addr, dst_addr);
  memmove (dst, src, size);
  preset->note_zones--;

  dec_size_zone = 0;
  if (note_zone->pri_zone != 0xff)
    dec_size_zone += sizeof (struct emu3_preset_zone);
  if (note_zone->sec_zone != 0xff)
    dec_size_zone += sizeof (struct emu3_preset_zone);

  addr = emu3_get_preset_zone_addr (file, preset_num);
  dst_addr = addr + sizeof (struct emu3_preset_zone) * zone_num;
  src_addr = dst_addr + dec_size_zone;
  src = &file->raw[src_addr];
  dst = &file->raw[dst_addr];
  size = file->size - dec_size_note_zone - src_addr;
  emu_debug (3, "Moving %d B from 0x%08x to 0x%08x...", size,
	     src_addr, dst_addr);
  memmove (dst, src, size);

  paddresses = emu3_get_preset_addresses (bank);
  max_presets = emu3_get_max_presets (bank);

  for (gint i = preset_num + 1; i < max_presets + 1; i++)
    paddresses[i] -= dec_size_note_zone + dec_size_zone;

  bank->next_preset -= dec_size_note_zone + dec_size_zone;
  file->size -= dec_size_note_zone + dec_size_zone;

  for (gint i = 0; i < EMU3_NOTES; i++)
    {
      if (preset->note_zone_mappings[i] == zone_num)
	preset->note_zone_mappings[i] = 0xff;
      else if (preset->note_zone_mappings[i] > zone_num &&
	       preset->note_zone_mappings[i] != 0xff)
	preset->note_zone_mappings[i]--;
    }

  return 0;
}

gint
emu3_add_preset (struct emu_file *file, gchar *preset_name, gint *preset_num)
{
  gint i, objects;
  guint32 copy_start_addr;
  struct emu3_bank *bank = EMU3_BANK (file);
  gint max_presets = emu3_get_max_presets (bank);
  guint32 *paddresses = emu3_get_preset_addresses (bank);
  guint32 next_sample_addr = emu3_get_next_sample_address (bank);
  void *src, *dst;

  objects = emu3_get_bank_samples (bank);

  for (i = 0; i < max_presets; i++)
    {
      if (paddresses[0] == paddresses[1])
	break;
      paddresses++;
    }

  if (i == max_presets)
    {
      emu_error ("No more presets allowed");
      return EXIT_FAILURE;
    }

  emu_debug (1, "Adding preset %d...", i);

  if (preset_num)
    {
      *preset_num = i;
    }
  objects += i;

  copy_start_addr = emu3_get_preset_address (bank, i);

  src = &file->raw[copy_start_addr];
  dst = &file->raw[copy_start_addr + sizeof (struct emu3_preset)];

  i++;
  paddresses++;

  for (; i < max_presets + 1; i++)
    {
      *paddresses += sizeof (struct emu3_preset);
      paddresses++;
    }

  gsize size = next_sample_addr - copy_start_addr;

  if (file->size + size > EMU3_MEM_SIZE)
    {
      emu_error ("Bank is full");
      return EXIT_FAILURE;
    }

  emu_debug (2, "Moving %zu B...", size);

  memmove (dst, src, size);

  struct emu3_preset *new_preset = (struct emu3_preset *) src;
  emu3_cpystr (new_preset->name, preset_name);
  memcpy (new_preset->rt_controls, DEFAULT_RT_CONTROLS,
	  RT_CONTROLS_SIZE + RT_CONTROLS_FS_SIZE);
  memset (new_preset->unknown_0, 0, PRESET_UNKNOWN_0_SIZE);
  new_preset->pitch_bend_range = 2;
  new_preset->velocity_range_pri_low = 0;
  new_preset->velocity_range_pri_high = 0;
  new_preset->velocity_range_sec_low = 0;
  new_preset->link_preset_lsb = 0;
  new_preset->link_preset_msb = 0;
  memset (new_preset->unknown_1, 0, PRESET_UNKNOWN_1_SIZE);
  new_preset->note_zones = 0;
  memset (new_preset->note_zone_mappings, 0xff, EMU3_NOTES);

  bank->objects = objects;
  bank->next_preset += sizeof (struct emu3_preset);
  bank->selected_preset = 0;
  file->size += sizeof (struct emu3_preset);

  return EXIT_SUCCESS;
}

gint
emu3_create_bank (const gchar *path, const gchar *type)
{
  gint rvalue;
  struct emu3_bank bank;
  gchar *dirc = strdup (path);
  gchar *basec = strdup (path);
  gchar *bname = basename (basec);

  gchar *name = emu3_str_to_emu3name (bname);
  gchar *src_path =
    malloc (strlen (DATADIR) + strlen (PACKAGE) +
	    strlen (EMPTY_BANK_TEMPLATE) + strlen (type) + 3);
  gint ret = sprintf (src_path, "%s/%s/%s%s", DATADIR, PACKAGE,
		      EMPTY_BANK_TEMPLATE, type);

  if (ret < 0)
    {
      emu_error ("Error while creating src path");
      rvalue = EXIT_FAILURE;
      goto out1;
    }

  FILE *src = fopen (src_path, "rb");
  if (!src)
    {
      emu_error ("Error while opening %s for input", src_path);
      rvalue = EXIT_FAILURE;
      goto out2;
    }

  FILE *dst = fopen (path, "w+b");
  if (!dst)
    {
      emu_error ("Error while opening %s for output", path);
      rvalue = EXIT_FAILURE;
      goto out3;
    }

  gchar buf[BUFSIZ];
  gsize size;

  while ((size = fread (buf, 1, BUFSIZ, src)))
    fwrite (buf, 1, size, dst);

  rewind (dst);

  if (fread (&bank, sizeof (struct emu3_bank), 1, dst))
    {
      emu3_cpystr (bank.name, name);
      emu3_cpystr (bank.name_copy, name);
      rewind (dst);
      fwrite (&bank, sizeof (struct emu3_bank), 1, dst);
    }

  emu_debug (2, "File created in %s", path);

  rvalue = EXIT_SUCCESS;

  fclose (dst);
out3:
  fclose (src);
out2:
  free (src_path);
out1:
  free (dirc);
  free (basec);
  free (name);
  return rvalue;
}

gint
emu3_write_file (struct emu_file *file)
{
  struct emu3_bank *bank = EMU3_BANK (file);
  guint32 sample_addr = emu3_get_sample_start_address (bank) - 1;
  guint32 preset = ceil (sample_addr / (gdouble) EMU3_BLOCK_SIZE);
  guint32 total = ceil (file->size / (gdouble) EMU3_BLOCK_SIZE);

  bank->total_blocks = htole32 (total);
  bank->preset_blocks = htole32 (preset);
  bank->sample_blocks = htole32 (total - preset);

  return emu_write_file (file);
}

static const gpointer
emu3_get_opcode_val (struct emu_sfz_context *esctx, const gchar *key)
{
  gchar *v = g_hash_table_lookup (esctx->region_opcodes, key);
  if (v)
    {
      return v;
    }
  v = g_hash_table_lookup (esctx->group_opcodes, key);
  if (v)
    {
      return v;
    }
  return g_hash_table_lookup (esctx->global_opcodes, key);
}

static const gpointer
emu3_get_opcode_val_with_alias (struct emu_sfz_context *esctx,
				const gchar *key, const gchar *alias)
{
  gpointer v = emu3_get_opcode_val (esctx, key);
  if (!v && alias)
    {
      v = emu3_get_opcode_val (esctx, alias);
    }
  return v;
}

static gdouble
emu3_get_opcode_number_val (struct emu_sfz_context *esctx, const gchar *key,
			    const gchar *alias, gdouble min, gdouble max,
			    gdouble def, gboolean *defined, gint decimals)
{
  gdouble v;
  const gdouble *val = emu3_get_opcode_val_with_alias (esctx, key, alias);
  if (defined)
    {
      *defined = val != NULL;
    }
  if (val)
    {
      v = *val;
      if (v < min)
	{
	  v = min;
	}
      else if (v > max)
	{
	  v = max;
	}
      if (v != *val)
	{
	  emu_debug (1,
		     "Value %.*f for opcode '%s' (alias or fallback '%s') outside range [ %.*f, %.*f ]. Using %.*f...",
		     decimals, v, key, alias, decimals, min, decimals, max,
		     decimals, v);
	}
    }
  else
    {
      v = def;
    }
  return v;
}

static gint64
emu3_get_opcode_integer_val (struct emu_sfz_context *esctx, const gchar *key,
			     const gchar *alias, gint64 min, gint64 max,
			     gint64 def, gboolean *defined)
{
  return emu3_get_opcode_number_val (esctx, key, alias, min, max, def,
				     defined, 0);
}

static gdouble
emu3_get_opcode_float_val (struct emu_sfz_context *esctx, const gchar *key,
			   const gchar *alias, gdouble min, gdouble max,
			   gdouble def, gboolean *defined)
{
  return emu3_get_opcode_number_val (esctx, key, alias, min, max, def,
				     defined, 2);
}

static const gchar *
emu3_get_opcode_string_val (struct emu_sfz_context *esctx, const gchar *key,
			    const gchar *alias, const gchar *def,
			    gboolean *defined)
{
  const gchar *v;
  const gchar *val = emu3_get_opcode_val_with_alias (esctx, key, alias);
  if (defined)
    {
      *defined = val != NULL;
    }
  if (val)
    {
      v = val;
    }
  else
    {
      v = def;
    }
  return v;
}

static void
emu_velocity_range_map_set (struct emu_velocity_range_map *vr, guint8 low,
			    guint8 high, gint preset_num)
{
  vr->low = low;
  vr->high = high;
  vr->preset_num = preset_num;
}

static void
emu_get_preset_name_with_layer (const gchar *preset_name, gint layer_num,
				gchar *name)
{
  snprintf (name, PATH_MAX, "%-12s L%02d", preset_name, layer_num);
}

static void
emu3_sfz_set_velocity_range_on (struct emu_sfz_context *esctx)
{
  if (esctx->velocity_layer_num > 1)
    {
      for (gint i = 0; i < EMU3_NOTES; i++)
	{
	  struct emu_velocity_range_map *vr =
	    &esctx->emu_velocity_range_maps[i];
	  struct emu3_preset *preset =
	    emu3_get_preset (esctx->file, vr->preset_num);
	  struct emu3_preset_note_zone *note_zone =
	    emu3_get_preset_note_zones (esctx->file, vr->preset_num);
	  for (gint j = 0; j < preset->note_zones; j++)
	    {
	      // These magic numbers activate the Velocity Ranges in the Crossfade/Switch preset menu.
	      note_zone->options_lsb = 0xc6;
	      note_zone->options_msb = 0xf9;
	      note_zone++;
	    }
	}
    }
}

static gint
emu_sfz_context_get_preset_by_velocity_range (struct emu_sfz_context *esctx,
					      guint8 low, guint8 high)
{
  gint err;
  gint new_preset_num;
  gchar name[PATH_MAX];
  struct emu3_preset *preset;
  struct emu_velocity_range_map *vr, *prev_vr;

  for (gint i = 0; i < EMU3_NOTES; i++)
    {
      vr = &esctx->emu_velocity_range_maps[i];
      if (vr->low == low && vr->high == high)
	{
	  emu_debug (1, "Using preset %d for velocity range [ %d, %d ]...",
		     vr->preset_num, low, high);
	  return vr->preset_num;
	}
    }

  if (esctx->velocity_layer_num + 1 >= EMU3_NOTES)
    {
      return -1;
    }

  if (esctx->velocity_layer_num == 0)
    {
      snprintf (name, PATH_MAX, "%s", esctx->preset_name);
    }
  else
    {
      emu_get_preset_name_with_layer (esctx->preset_name,
				      esctx->velocity_layer_num, name);
    }
  emu_debug (1, "Creating preset '%s' for velocity range [ %d, %d ]...",
	     name, low, high);
  err = emu3_add_preset (esctx->file, name, &new_preset_num);
  if (err)
    {
      return err;
    }

  preset = emu3_get_preset (esctx->file, new_preset_num);
  preset->velocity_range_pri_low = low;
  preset->velocity_range_pri_high = high;
  vr = &esctx->emu_velocity_range_maps[esctx->velocity_layer_num];
  emu_velocity_range_map_set (vr, low, high, new_preset_num);

  // Link previous preset to the current one
  if (esctx->velocity_layer_num > 0)
    {
      prev_vr =
	&esctx->emu_velocity_range_maps[esctx->velocity_layer_num - 1];
      gint prev_preset_num = prev_vr->preset_num;
      preset = emu3_get_preset (esctx->file, prev_preset_num);
      gint p = vr->preset_num + 1;
      preset->link_preset_lsb = p & 0xff;
      preset->link_preset_msb = (p >> 8) & 0xff;

      //In case we have more than one preset, overwrite the name of the first one and set the Velocity Ranges on.
      if (esctx->velocity_layer_num == 1)
	{
	  emu_get_preset_name_with_layer (esctx->preset_name, 0, name);
	  memcpy (preset->name, name, EMU3_NAME_SIZE);
	}
    }

  esctx->velocity_layer_num++;

  return new_preset_num;
}

static void
emu_replace_backslashes_in_path (gchar *sample_path)
{
  gchar *c = sample_path;
//Perhaps converting backslashes to slashes is not a good idea but it's practical.
  while (*c)
    {
      if (*c == '\\')
	{
	  *c = '/';
	}
      c++;
    }
}

static void
emu3_sfz_set_envelope (struct emu_sfz_context *esctx, struct emu3_envelope *e,
		       const gchar *attack_opcode,
		       const gchar *hold_opcode,
		       const gchar *decay_opcode,
		       const gchar *sustain_opcode,
		       const gchar *release_opcode)
{
  gdouble v;

  v = emu3_get_opcode_float_val (esctx, attack_opcode, NULL, 0, 100, 0, NULL);
  e->attack = emu3_get_u8_from_time_163_69 (v);
  v = emu3_get_opcode_float_val (esctx, hold_opcode, NULL, 0, 100, 0, NULL);
  e->hold = emu3_get_u8_from_time_163_69 (v);
  v = emu3_get_opcode_float_val (esctx, decay_opcode, NULL, 0, 100, 0, NULL);
  e->decay = emu3_get_u8_from_time_163_69 (v);
  v = emu3_get_opcode_float_val (esctx, sustain_opcode, NULL, 0, 100, 100,
				 NULL);
  e->sustain = emu3_get_s8_from_percent (v);
  v = emu3_get_opcode_float_val (esctx, release_opcode, NULL, 0, 100, 0.001,
				 NULL);
  e->release = emu3_get_u8_from_time_163_69 (v);
}

struct emu3_sfz_fil_type_map
{
  const gchar *sfz_fil_type;
  gint emu3_filter_id;		// -1 means not implemented
};

const struct emu3_sfz_fil_type_map EMU3_SFZ_FIL_TYPE_MAPS[] = {
  {"bpf_1p", -1},
  {"brf_1p", -1},
  {"apf_1p", 11},		// Phaser 1
  {"hpf_1p", -1},
  {"lpf_1p", -1},
  {"bpf_2p", 5},
  {"brf_2p", 7},
  {"hpf_2p", 3},
  {"lpf_2p", 0},
  {"pkf_2p", -1},
  {"bpf_2p_sv", -1},
  {"brf_2p_sv", -1},
  {"hpf_2p_sv", -1},
  {"lpf_2p_sv", -1},
  {"hpf_4p", 4},
  {"lpf_4p", 1},
  {"hpf_6p", -1},
  {"lpf_6p", 2},
  {"comb", 14},			// Flanger Lite
  {"pink", -1}
};

static const gint EMU3_SFZ_FIL_TYPE_MAPS_SIZE =
  sizeof (EMU3_SFZ_FIL_TYPE_MAPS) / sizeof (struct emu3_sfz_fil_type_map);

gint
emu3_get_filter_id_from_sfz_fil_type (const gchar *sfz_fil_type)
{
  gint emu3_filter_id = -1;
  const struct emu3_sfz_fil_type_map *map = EMU3_SFZ_FIL_TYPE_MAPS;
  for (gint i = 0; i < EMU3_SFZ_FIL_TYPE_MAPS_SIZE; i++, map++)
    {
      if (!strcmp (map->sfz_fil_type, sfz_fil_type))
	{
	  emu3_filter_id = map->emu3_filter_id;
	  break;
	}
    }
  if (emu3_filter_id == -1)
    {
      emu_debug (1, "SFZ filter type '%s' has no EMU3 filter correspondence",
		 sfz_fil_type);
      emu3_filter_id = 0;
    }
  emu_debug (1, "Using filter '%s'...", VCF_TYPE[emu3_filter_id]);
  return emu3_filter_id;
}

static void
emu3_set_sample_options_from_sfz_loop_mode (struct emu3_sample *sample,
					    const gchar *loop_mode)
{
  guint16 options_wo_loop = sample->options & ~EMU3_SAMPLE_OPT_LOOP_MASK;
  if (!strcmp ("no_loop", loop_mode) || !strcmp ("one_shot", loop_mode))
    {
      sample->options = options_wo_loop;
    }
  else if (!strcmp ("loop_continuous", loop_mode))
    {
      sample->options = options_wo_loop | EMU3_SAMPLE_OPT_LOOP |
	EMU3_SAMPLE_OPT_LOOP_RELEASE;
    }
  else if (!strcmp ("loop_sustain", loop_mode))
    {
      sample->options = options_wo_loop | EMU3_SAMPLE_OPT_LOOP;
    }
  else
    {
      if (sample->options & EMU3_SAMPLE_OPT_LOOP)
	{
	  emu_debug (1, "Setting loop mode to 'loop_continuous'...");
	  sample->options = options_wo_loop | EMU3_SAMPLE_OPT_LOOP |
	    EMU3_SAMPLE_OPT_LOOP_RELEASE;
	}
      else
	{
	  emu_debug (1, "Setting loop mode to 'no_loop'...");
	  sample->options = options_wo_loop;
	}
    }
}

void
emu3_sfz_add_region (struct emu_sfz_context *esctx)
{
  gdouble f;
  const gchar *s;
  gchar *sample_path;
  struct emu_file *file;
  struct emu3_bank *bank;
  gboolean mono, defined;
  struct emu3_preset_zone *zone;
  struct emu3_sample *emu3_sample;
  struct emu_zone_range zone_range;
  const gchar *sample, *fil_type_def;
  guint32 frames, loop_start, loop_end;
  gint err, sample_num, actual_preset, i;
  gint lokey, hikey, pitch_keycenter, lovel, hivel;

  sample = emu3_get_opcode_string_val (esctx, "sample", NULL, NULL, NULL);
  if (!sample)
    {
      emu_error ("No 'sample' opcode found in region");
      return;
    }

  file = esctx->file;
  bank = EMU3_BANK (file);

  lokey = emu3_get_opcode_integer_val (esctx, "lokey", "key",
				       EMU3_LOWEST_MIDI_NOTE,
				       EMU3_HIGHEST_MIDI_NOTE,
				       EMU3_LOWEST_MIDI_NOTE, NULL);
  hikey = emu3_get_opcode_integer_val (esctx, "hikey", "key",
				       EMU3_LOWEST_MIDI_NOTE,
				       EMU3_HIGHEST_MIDI_NOTE,
				       EMU3_HIGHEST_MIDI_NOTE, NULL);
  pitch_keycenter = emu3_get_opcode_integer_val (esctx, "pitch_keycenter",
						 "key", EMU3_LOWEST_MIDI_NOTE,
						 EMU3_HIGHEST_MIDI_NOTE, 60,
						 NULL);
  lovel = emu3_get_opcode_integer_val (esctx, "lovel", NULL, 1, 127, 1, NULL);
  hivel = emu3_get_opcode_integer_val (esctx, "hivel", NULL, 1, 127, 127,
				       NULL);

  emu_debug (1,
	     "Processing region %02d for '%s' (pitch_keycenter: %d; lokey: %d; hikey: %d; lovel: %d, hivel: %d)...",
	     esctx->region_num, sample, pitch_keycenter, lokey, hikey, lovel,
	     hivel);

  actual_preset = emu_sfz_context_get_preset_by_velocity_range (esctx, lovel,
								hivel);
  if (actual_preset < 0)
    {
      emu_error ("Error while getting the preset");
      return;
    }

  emu_debug (1,
	     "Adding region for '%s' centered at %d [ %d, %d ] with velocity in [ %d, %d ]...",
	     sample, pitch_keycenter, lokey, hikey, lovel, hivel);

  // Notice that SFZ states that MIDI EMU3_NOTES go from C-1 to G9. See https://sfzformat.com/opcodes/sw_hikey/.
  // Therefore the lowest A on a piano is A0 in the SFZ format while it is A-1 in the E-mu.
  // 0 is A-1

  zone_range.layer = 1;		//Always using pri layer
  zone_range.original_key = pitch_keycenter - EMU3_MIDI_NOTE_OFFSET;
  zone_range.lower_key = lokey - EMU3_MIDI_NOTE_OFFSET;
  zone_range.higher_key = hikey - EMU3_MIDI_NOTE_OFFSET;

  sample_path = g_strdup_printf ("%s/%s", esctx->sfz_dir, sample);
  emu_replace_backslashes_in_path (sample_path);
  err = emu3_add_sample (esctx->file, sample_path, &sample_num, &mono,
			 &frames);
  g_free (sample_path);
  if (err)
    {
      return;
    }

  if (emu3_add_preset_zone (esctx->file, actual_preset,
			    sample_num, &zone_range, &zone))
    {
      return;
    }

  // Region opcodes

  // sample

  err = emu3_get_sample (esctx->file, sample_num, &emu3_sample);
  if (err)
    {
      return;
    }
  fil_type_def = emu3_sample->options & EMU3_SAMPLE_OPT_LOOP ?
    "loop_continuous" : "no_loop";
  s = emu3_get_opcode_string_val (esctx, "loop_mode", NULL, fil_type_def,
				  NULL);
  emu3_set_sample_options_from_sfz_loop_mode (emu3_sample, s);

  loop_start = emu3_get_opcode_integer_val (esctx, "loop_start", NULL, 0,
					    G_MAXUINT32, 0, &defined);
  if (defined)
    {
      emu3_sample_set_loop_start (emu3_sample, mono, frames, loop_start);
    }
  loop_end = emu3_get_opcode_integer_val (esctx, "loop_end", NULL, 0,
					  G_MAXUINT32, 0, &defined);
  if (defined)
    {
      emu3_sample_set_loop_end (emu3_sample, mono, frames, loop_end);
    }

  f = emu3_get_opcode_integer_val (esctx, "tune", "pitch", -100, 100, 0,
				   NULL);
  zone->note_tuning = emu3_s8_from_get_note_tuning (f);

  // VCA and pan

  f = emu3_get_opcode_float_val (esctx, "amp_veltrack", NULL, -100, 100,
				 100, NULL);
  zone->vel_to_vca_level = emu3_get_s8_from_percent (f);

  emu3_sfz_set_envelope (esctx, &zone->vca_envelope, "ampeg_attack",
			 "ampeg_hold", "ampeg_decay", "ampeg_sustain",
			 "ampeg_release");

  f = emu3_get_opcode_float_val (esctx, "pan", NULL, -100, 100, 0, NULL);
  zone->vca_pan = emu3_get_s8_from_percent_signed (f);

  f = emu3_get_opcode_float_val (esctx, "pan_veltrack", NULL, -100, 100, 0,
				 NULL);
  zone->vel_to_pan = emu3_get_s8_from_percent (f);

  // VCF

  s = emu3_get_opcode_string_val (esctx, "fil_type", NULL, "lpf_2p", NULL);
  i = emu3_get_filter_id_from_sfz_fil_type (s);
  emu3_set_preset_zone_filter (zone, i);

  f = emu3_get_opcode_float_val (esctx, "cutoff", NULL,
				 TABLE_VCF_CUTOFF_FREQUENCY[0],
				 TABLE_VCF_CUTOFF_FREQUENCY[255],
				 emu3_get_vcf_cutoff_frequency_from_u8
				 (DEFAULT_CUTOFF_U8), NULL);
  zone->vcf_cutoff = emu3_get_u8_from_vcf_cutoff_frequency (f);

  // Probably, the value mapping is not right as the whole SFZ range, which is
  // [ 0, 40 ] dB, is mapped to the whole output range, which is a percentage.
  f = emu3_get_opcode_float_val (esctx, "resonance", NULL, 0, 40, 0, NULL);
  zone->vcf_q = emu3_get_s8_from_percent (f * 2.5) |
    (strcmp (ESI_32_V3_DEF, bank->format) == 0 ? 0x80 : 0);

  emu3_sfz_set_envelope (esctx, &zone->vcf_envelope, "fileg_attack",
			 "fileg_hold", "fileg_decay", "fileg_sustain",
			 "fileg_release");

  // The value mapping is just an approximation as the device uses a percentage.
  f = emu3_get_opcode_float_val (esctx, "fileg_depth", NULL, -12000, 12000, 0,
				 NULL);
  zone->vcf_envelope_amount = emu3_get_s8_from_percent (f / 120);

  // The value mapping is just an approximation as the device uses a percentage.
  f = emu3_get_opcode_float_val (esctx, "fil_veltrack", NULL, -9600, 9600, 0,
				 NULL);
  zone->vel_to_vcf_cutoff = emu3_get_s8_from_percent (f / 96.0);

  // The range in the specification is smaller than the one one in the device,
  // which is [ -2.0, 2.0 ] or [ -2400, 2400 ].
  f = emu3_get_opcode_float_val (esctx, "fil_keytrack", NULL, 0, 1200,
				 0, NULL);
  zone->vcf_tracking = emu3_get_s8_from_vcf_tracking (f / 1200.0);

  esctx->region_num++;
}

// This functions sets opcodes that in the hardware can only be set at preset level.

static void
emu3_sfz_set_preset_opcodes (struct emu_sfz_context *esctx)
{
  gint bend_up, v;
  struct emu3_preset *preset;
  struct emu_velocity_range_map *vr;

  // This is required to not ignore the opcodes from the last read region.
  g_hash_table_remove_all (esctx->region_opcodes);

  bend_up = emu3_get_opcode_integer_val (esctx, "bend_up", "bendup",
					 -9600, 9600, 200, NULL);
  v = bend_up / 100;

  for (gint i = 0; i < EMU3_NOTES; i++)
    {
      vr = &esctx->emu_velocity_range_maps[i];
      if (vr->preset_num == -1)
	{
	  break;
	}
      preset = emu3_get_preset (esctx->file, vr->preset_num);
      emu3_set_preset_pbr (preset, v);
    }
}

gint
emu3_add_sfz (struct emu_file *file, const gchar *sfz_path)
{
  gint err;
  FILE *sfz;
  const gchar *ext;
  struct emu_sfz_context esctx;
  gchar *sfz_name, *bnsfz, *preset_name, *sfz_dir, *bdsfz;

  bdsfz = strdup (sfz_path);
  sfz_dir = dirname (bdsfz);
  bnsfz = strdup (sfz_path);
  sfz_name = basename (bnsfz);
  preset_name = emu_filename_to_filename_wo_ext (sfz_name, &ext);
  if (strcasecmp (ext, "sfz"))
    {
      emu_debug (1, "Unexpected extension \"%s\"", ext);
    }

  sfz = fopen (sfz_path, "rb");
  if (!sfz)
    {
      emu_error ("Error: %s", strerror (errno));
      err = EXIT_FAILURE;
      goto end;
    }

  yyset_in (sfz);

  esctx.file = file;
  esctx.velocity_layer_num = 0;
  esctx.preset_name = preset_name;
  esctx.region_num = 0;
  esctx.sfz_dir = sfz_dir;
  esctx.global_opcodes = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, g_free);
  esctx.group_opcodes = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free, g_free);
  esctx.region_opcodes = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, g_free);
  for (gint i = 0; i < EMU3_NOTES; i++)
    {
      struct emu_velocity_range_map *vr = &esctx.emu_velocity_range_maps[i];
      emu_velocity_range_map_set (vr, 0xff, 0xff, -1);
    }

  sfz_parser_set_context (&esctx);

  // Process regions
  yyparse ();

  emu3_sfz_set_velocity_range_on (&esctx);

  // Preset opcodes

  emu3_sfz_set_preset_opcodes (&esctx);

  g_hash_table_unref (esctx.global_opcodes);
  g_hash_table_unref (esctx.group_opcodes);
  g_hash_table_unref (esctx.region_opcodes);

  fclose (sfz);

  err = emu3_write_file (file);

end:
  free (bdsfz);
  free (bnsfz);
  free (preset_name);
  return err;
}
