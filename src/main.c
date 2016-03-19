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

#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include "emu3bm.h"

int
main (int argc, char *argv[])
{
  int c;
  int xflg = 0, aflg = 0, cflg = 0, errflg = 0;
  char *ifile;
  char *afile;
  extern char *optarg;
  extern int optind, optopt;

  while ((c = getopt (argc, argv, "a:xc")) != -1)
    {
      switch (c)
	{
	case ':':
	  ifile = optarg;
	  break;
	case 'a':
	  if (xflg || cflg)
	    {
	      errflg++;
	    }
	  else
	    {
	      aflg++;
	      afile = optarg;
	    }
	  break;
	case 'x':
	  if (aflg || cflg)
	    {
	      errflg++;
	    }
	  else
	    {
	      xflg++;
	    }
	  break;
	case 'c':
	  if (aflg || xflg)
	    {
	      errflg++;
	    }
	  else
	    {
	      cflg++;
	    }
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

  if (errflg > 0)
    {
      fprintf (stderr, "%s\n", PACKAGE_STRING);
      fprintf (stderr, "Usage: %s [-x | -a append_file | -c ] input_file.\n",
	       basename (argv[0]));
      return -1;
    }

  if (xflg || aflg)
    {
      return emu3_process_bank (ifile, aflg, afile, xflg);
    }
  else if (cflg)
    {
      return emu3_create_bank (ifile);
    }
}
