/*
 *   main_emu3bm.c
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
#include <stdlib.h>
#include <string.h>
#include "emu3bm.h"

static const struct option options[] = {
  {"pitch-bend-range", 1, NULL, 'b'},
  {"bit-depth", 1, NULL, 'B'},
  {"filter-cutoff", 1, NULL, 'c'},
  {"device-type", 1, NULL, 'd'},
  {"preset-to-edit", 1, NULL, 'e'},
  {"filter-type", 1, NULL, 'f'},
  {"help", 0, NULL, 'h'},
  {"level", 1, NULL, 'l'},
  {"new-bank", 1, NULL, 'n'},
  {"add-preset", 1, NULL, 'p'},
  {"filter-q", 1, NULL, 'q'},
  {"real-time-controls", 1, NULL, 'r'},
  {"max-sample-rate", 1, NULL, 'R'},
  {"add-sample", 1, NULL, 's'},
  {"add-preset-from-sfz", 1, NULL, 'S'},
  {"verbosity", 0, NULL, 'v'},
  {"extract-samples", 0, NULL, 'x'},
  {"extract-samples-with-num", 0, NULL, 'X'},
  {"delete-zone", 1, NULL, 'y'},
  {"add-zone", 1, NULL, 'z'},
  {"add-zone-with-num", 1, NULL, 'Z'},
  {NULL, 0, NULL, 0}
};

static int
parse_zone_params (char *zone_params, int *sample_num,
		   struct emu3_zone_range *zone_range, int is_num)
{
  char *sample_str = strsep (&zone_params, ",");
  char *layer = strsep (&zone_params, ",");
  char *original_key = strsep (&zone_params, ",");
  char *lower_key = strsep (&zone_params, ",");
  char *higher_key = strsep (&zone_params, ",");
  char *endtoken;

  *sample_num = strtol (sample_str, &endtoken, 10);
  if (*endtoken != '\0' || sample_num <= 0)
    {
      emu_error ("Invalid sample %d", *sample_num);
      return EXIT_FAILURE;
    }

  int orig_key_int;
  if (is_num)
    orig_key_int = strtol (original_key, &endtoken, 10);
  else
    orig_key_int = emu_reverse_note_search (original_key);
  if (orig_key_int == -1 || orig_key_int < 0 || orig_key_int >= NOTES)
    {
      emu_error ("Invalid original key %s", original_key);
      return EXIT_FAILURE;
    }
  zone_range->original_key = orig_key_int;

  int lower_key_int;
  if (is_num)
    lower_key_int = strtol (lower_key, &endtoken, 10);
  else
    lower_key_int = emu_reverse_note_search (lower_key);
  if (lower_key_int == -1 || lower_key_int < 0 || lower_key_int >= NOTES)
    {
      emu_error ("Invalid lower key %s", lower_key);
      return EXIT_FAILURE;
    }
  zone_range->lower_key = lower_key_int;

  int higher_key_int;
  if (is_num)
    higher_key_int = strtol (higher_key, &endtoken, 10);
  else
    higher_key_int = emu_reverse_note_search (higher_key);
  if (higher_key_int == -1 || higher_key_int < 0 || higher_key_int >= NOTES)
    {
      emu_error ("Invalid higher key %s", higher_key);
      return EXIT_FAILURE;
    }
  zone_range->higher_key = higher_key_int;

  if (!strcmp ("pri", layer))
    zone_range->layer = 1;
  else if (!strcmp ("sec", layer))
    zone_range->layer = 2;
  else
    {
      emu_error ("Invalid layer %s", layer);
      return EXIT_FAILURE;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  int opt;
  int long_index = 0;
  int xflg = 0, dflg = 0, sflg = 0, nflg = 0, sfzflg = 0, errflg = 0, modflg =
    0, pflg = 0, zflg = 0, yflg = 0, ext_mode = EMU3_EXT_MODE_NONE;
  char *device = NULL;
  char *bank_name = NULL;
  char *sample_name;
  char *sfz_filename;
  char *preset_name;
  char *rt_controls = NULL;
  char *zone_params = NULL;
  int level = -1;
  int cutoff = -1;
  int q = -1;
  int filter = -1;
  int pbr = -1;
  int preset_num = -1;
  extern char *optarg;
  extern int optind, optopt;
  int err = 0;
  int sample_num;
  struct emu3_zone_range zone_range;
  int zone_num;

  while ((opt = getopt_long (argc, argv,
			     "b:B:c:d:e:f:hl:np:q:r:R:s:S:vxXy:z:Z:", options,
			     &long_index)) != -1)
    {
      switch (opt)
	{
	case 'b':
	  pbr = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'B':
	  bit_depth = get_positive_int_in_range (optarg,
						 MIN_BIT_DEPTH,
						 MAX_BIT_DEPTH);
	  if (bit_depth < 0)
	    {
	      exit (err);
	    }
	  break;
	case 'c':
	  cutoff = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'd':
	  dflg++;
	  device = optarg;
	  break;
	case 'e':
	  preset_num = get_positive_int (optarg);
	  break;
	case 'f':
	  filter = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'h':
	  emu_print_help (argv[0], PACKAGE_STRING, options);
	  exit (EXIT_SUCCESS);
	case 'l':
	  level = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'n':
	  nflg++;
	  break;
	case 'p':
	  preset_name = optarg;
	  pflg++;
	  break;
	case 'q':
	  q = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'r':
	  rt_controls = optarg;
	  modflg++;
	  break;
	case 'R':
	  max_sample_rate = get_positive_int_in_range (optarg,
						       MIN_SAMPLE_RATE,
						       MAX_SAMPLE_RATE);
	  if (max_sample_rate < 0)
	    {
	      exit (err);
	    }
	  break;
	case 's':
	  sflg++;
	  sample_name = optarg;
	  break;
	case 'S':
	  sfzflg++;
	  sfz_filename = optarg;
	  break;
	case 'v':
	  verbosity++;
	  break;
	case 'x':
	  xflg++;
	  ext_mode = EMU3_EXT_MODE_NAME;
	  break;
	case 'X':
	  xflg++;
	  ext_mode = EMU3_EXT_MODE_NAME_NUMBER;
	  break;
	case 'y':
	  zone_num = get_positive_int (optarg);
	  yflg++;
	  break;
	case 'z':
	case 'Z':
	  zone_params = optarg;
	  int err = parse_zone_params (zone_params, &sample_num, &zone_range,
				       opt == 'Z');
	  if (err)
	    exit (err);
	  zflg++;
	  break;
	case '?':
	  errflg++;
	}
    }

  if (optind + 1 == argc)
    bank_name = argv[optind];
  else
    errflg++;

  if (!device)
    device = DEVICE_ESI2000;
  else if (strcmp (device, DEVICE_ESI2000) && strcmp (device, DEVICE_EMU3X))
    errflg++;

  if (dflg > 1)
    errflg++;

  if (dflg && !nflg)
    errflg++;

  if (nflg > 1)
    errflg++;

  if (sflg > 1)
    errflg++;

  if (pflg > 1)
    errflg++;

  if (zflg > 1)
    errflg++;

  if (yflg > 1)
    errflg++;

  if (sfzflg > 1)
    errflg++;

  if (nflg + sflg + pflg + zflg + yflg + sfzflg > 1)
    errflg++;

  if ((nflg || sflg || pflg || zflg || yflg || sfzflg) && modflg)
    errflg++;

  if (errflg > 0)
    {
      emu_print_help (argv[0], PACKAGE_STRING, options);
      exit (EXIT_FAILURE);
    }

  if (nflg)
    {
      err = emu3_create_bank (bank_name, device);
      exit (err);
    }

  struct emu_file *file = emu3_open_file (bank_name);
  if (!file)
    exit (EXIT_FAILURE);

  if (sflg)
    {
      err = emu3_add_sample (file, sample_name, NULL);
      goto end;
    }

  if (pflg)
    {
      err = emu3_add_preset (file, preset_name, NULL);
      goto end;
    }

  if (zflg)
    {
      err = emu3_add_preset_zone (file, preset_num, sample_num,
				  &zone_range, NULL);
      goto end;
    }

  if (yflg)
    {
      err = emu3_del_preset_zone (file, preset_num, zone_num);
      goto end;
    }

  if (sfzflg)
    {
      err = emu3_add_sfz (file, sfz_filename);
      goto end;
    }

  err = emu3_process_bank (file, ext_mode, preset_num, rt_controls, pbr,
			   level, cutoff, q, filter);

end:
  if (err)
    {
      goto close;
    }

  if (sflg || pflg || zflg || yflg || modflg)
    {
      err = emu3_write_file (file);
    }

close:
  emu_close_file (file);
  exit (err);
}
