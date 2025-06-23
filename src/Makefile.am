AM_CFLAGS = -I$(top_srcdir)/src `$(PKG_CONFIG) --cflags $(LIBS)` $(SNDFILE_CFLAGS)
emu3bm_LDADD = $(SNDFILE_LIBS) -lm
emu4bm_LDADD = $(SNDFILE_LIBS) -lm

bin_PROGRAMS = emu3bm emu4bm
emu3bm_SOURCES = main_emu3bm.c emu3bm.c emu3bm.h sample.c sample.h utils.c utils.h
emu4bm_SOURCES = main_emu4bm.c sample.c sample.h utils.c utils.h

SNDFILE_CFLAGS = @SNDFILE_CFLAGS@
SNDFILE_LIBS = @SNDFILE_LIBS@

AM_CPPFLAGS = -DDATADIR='"$(datadir)"'
