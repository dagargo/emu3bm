/*
 *	main.c
 *	Copyright (C) 2018 David García Goñi <dagargo at gmail dot com>
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

#include "emu3bm.h"

int verbosity = 0;

int
get_positive_int (char *str)
{
  char *endstr;

  int value = (int) strtol (str, &endstr, 10);

  if (errno || endstr == str || *endstr != '\0')
    {
      fprintf (stderr, "Value %s not valid\n", str);
      value = -1;
    }
  return value;
}

int
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
      fprintf (stderr, "Illegal sample %d.\n", sample_num);
      return EXIT_FAILURE;
    }

  int orig_key_int;
  if (is_num)
    orig_key_int = strtol (original_key, &endtoken, 10);
  else
    orig_key_int = emu3_reverse_note_search (original_key);
  if (orig_key_int == -1 || orig_key_int < 0 || orig_key_int >= NOTES)
    {
      fprintf (stderr, "Illegal original key %s.\n", original_key);
      return EXIT_FAILURE;
    }
  zone_range->original_key = orig_key_int;

  int lower_key_int;
  if (is_num)
    lower_key_int = strtol (lower_key, &endtoken, 10);
  else
    lower_key_int = emu3_reverse_note_search (lower_key);
  if (lower_key_int == -1 || lower_key_int < 0 || lower_key_int >= NOTES)
    {
      fprintf (stderr, "Illegal lower key %s.\n", lower_key);
      return EXIT_FAILURE;
    }
  zone_range->lower_key = lower_key_int;

  int higher_key_int;
  if (is_num)
    higher_key_int = strtol (higher_key, &endtoken, 10);
  else
    higher_key_int = emu3_reverse_note_search (higher_key);
  if (higher_key_int == -1 || higher_key_int < 0 || higher_key_int >= NOTES)
    {
      fprintf (stderr, "Illegal higher key %s.\n", higher_key);
      return EXIT_FAILURE;
    }
  zone_range->higher_key = higher_key_int;

  if (!strcmp ("pri", layer))
    zone_range->layer = 1;
  else if (!strcmp ("sec", layer))
    zone_range->layer = 2;
  else
    {
      fprintf (stderr, "Invalid layer %s.\n", layer);
      return EXIT_FAILURE;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  int c;
  int xflg = 0, sflg = 0, nflg = 0, errflg = 0, modflg = 0, pflg = 0, zflg =
    0, ext_mode = 0;
  char *bank_filename;
  char *sample_filename;
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
  int result = 0;
  int loop;
  int sample_num;
  struct emu3_zone_range zone_range;

  while ((c = getopt (argc, argv, "vns:S:xXr:l:c:q:f:b:e:p:z:Z:")) != -1)
    {
      switch (c)
	{
	case 'v':
	  verbosity++;
	  break;
	case 's':
	  sflg++;
	  sample_filename = optarg;
	  loop = 0;
	  break;
	case 'S':
	  sflg++;
	  sample_filename = optarg;
	  loop = 1;
	  break;
	case 'x':
	  xflg++;
	  ext_mode = 1;
	  break;
	case 'X':
	  xflg++;
	  ext_mode = 2;
	  break;
	case 'n':
	  nflg++;
	  break;
	case 'r':
	  rt_controls = optarg;
	  modflg++;
	  break;
	case 'l':
	  level = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'c':
	  cutoff = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'q':
	  q = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'f':
	  filter = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'b':
	  pbr = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'e':
	  preset_num = get_positive_int (optarg);
	  break;
	case 'p':
	  preset_name = optarg;
	  pflg++;
	  break;
	case 'z':
	case 'Z':
	  zone_params = optarg;
	  int result =
	    parse_zone_params (zone_params, &sample_num, &zone_range,
			       c == 'Z');
	  if (result)
	    exit (result);
	  zflg++;
	  break;
	case '?':
	  fprintf (stderr, "Unrecognized option: -%c\n", optopt);
	  errflg++;
	}
    }

  if (optind + 1 == argc)
    bank_filename = argv[optind];
  else
    errflg++;

  if (nflg > 1)
    errflg++;

  if (sflg > 1)
    errflg++;

  if (pflg > 1)
    errflg++;

  if (zflg > 1)
    errflg++;

  if (nflg + sflg + pflg + zflg > 1)
    errflg++;

  if ((nflg || sflg || pflg || zflg) && modflg)
    errflg++;

  if (errflg > 0)
    {
      fprintf (stderr, "%s\n", PACKAGE_STRING);
      char *runnable = basename (argv[0]);
      fprintf (stderr, "Usage: %s [OPTIONS] bank_file.\n", runnable);
      exit (EXIT_FAILURE);
    }

  if (nflg)
    {
      result = emu3_create_bank (bank_filename);
      exit (result);
    }

  struct emu3_file *file = emu3_open_file (bank_filename);

  if (!file)
    exit (EXIT_FAILURE);

  if (sflg)
    {
      result = emu3_add_sample (file, sample_filename, loop);
      goto close;
    }

  if (pflg)
    {
      result = emu3_add_preset (file, preset_name);
      goto close;
    }

  if (zflg)
    {
      result =
	emu3_add_preset_zone (file, preset_num, sample_num, &zone_range);
      goto close;
    }

  if (!nflg && !sflg && !pflg && !zflg)
    result =
      emu3_process_bank (file, ext_mode, preset_num, rt_controls, pbr, level,
			 cutoff, q, filter);

close:
  if (sflg || pflg || zflg || modflg)
    emu3_write_file (file);
  emu3_close_file (file);
  exit (result);
}
