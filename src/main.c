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

int
main (int argc, char *argv[])
{
  int c;
  int xflg = 0, aflg = 0, cflg = 0, errflg = 0;
  char *ifile;
  char *afile;
  char *rt_controls = NULL;
  extern char *optarg;
  extern int optind, optopt;

  while ((c = getopt (argc, argv, "a:xcr:")) != -1)
    {
      switch (c)
	{
	case 'a':
	  aflg++;
	  afile = optarg;
	  break;
	case 'x':
	  xflg++;
	  break;
	case 'c':
	  cflg++;
	  break;
	case 'r':
	  rt_controls = optarg;
	  break;
	case '?':
	  fprintf (stderr, "Unrecognized option: -%c\n", optopt);
	  errflg++;
	}
    }

  if (optind + 1 == argc)
    {
      ifile = argv[optind];
    }
  else
    {
      errflg++;
    }

  if (cflg && (xflg || aflg || rt_controls))
    {
      errflg++;
    }

  if (errflg > 0)
    {
      fprintf (stderr, "%s\n", PACKAGE_STRING);
      fprintf (stderr, "Usage: %s [OPTIONS] input_file.\n",
	       basename (argv[0]));
      exit (EXIT_FAILURE);
    }

  if (cflg)
    {
      exit (emu3_create_bank (ifile));

    }
  else
    {
      exit (emu3_process_bank (ifile, aflg, afile, xflg, rt_controls));
    }
}
