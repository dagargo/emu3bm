/*
 *	main.c
 *	Copyright (C) 2016 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   EMU3 Filesystem Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   EMU3 Filesystem Tool is distributed in the hope that it will be useful,
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
main (int argc, char *argv[])
{
  int c;
  int xflg = 0, aflg = 0, nflg = 0, errflg = 0, modflg = 0;
  char *bank_filename;
  char *sample_filename;
  char *rt_controls = NULL;
  int level = -1;
  int cutoff = -1;
  int q = -1;
  int filter = -1;
  int pbr = -1;
  int preset = -1;
  extern char *optarg;
  extern int optind, optopt;
  int result = 0;

  while ((c = getopt (argc, argv, "vna:xr:l:c:q:f:p:P:")) != -1)
    {
      switch (c)
	{
	case 'v':
	  verbosity++;
	  break;
	case 'a':
	  aflg++;
	  sample_filename = optarg;
	  break;
	case 'x':
	  xflg++;
	  modflg++;
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
	case 'p':
	  pbr = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'P':
	  preset = get_positive_int (optarg);
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

  if ((nflg || aflg) && modflg)
    errflg++;

  if (errflg > 0)
    {
      fprintf (stderr, "%s\n", PACKAGE_STRING);
      fprintf (stderr, "Usage: %s [OPTIONS] input_file.\n",
	       basename (argv[0]));
      exit (EXIT_FAILURE);
    }

  if (nflg)
    result = emu3_create_bank (bank_filename);

  if (result)
    exit (result);

  struct emu3_file *file = emu3_open_file (bank_filename);

  if (!file)
    exit (result);

  if (aflg)
    {
      result = emu3_add_sample (file, sample_filename);

      if (result)
	{
	  emu3_close_file (file);
	  exit (result);
	}
    }

  if (!nflg && !aflg)
    {
      result =
	emu3_process_bank (file, preset, xflg, rt_controls, level,
			   cutoff, q, filter, pbr);

      if (result)
	{
	  emu3_close_file (file);
	  exit (result);
	}
    }

  if (aflg || modflg)
    emu3_write_file (file);

  emu3_close_file (file);

  exit (result);
}
