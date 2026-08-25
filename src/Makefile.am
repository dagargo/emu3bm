PKG_CONFIG ?= pkg-config

AM_CFLAGS = -Wall -O3
AM_CPPFLAGS = -DDATADIR='"$(datadir)/$(PACKAGE)"'

DEP_LIBS = glib-2.0

emu3bm_CFLAGS = -I$(top_srcdir)/src `$(PKG_CONFIG) --cflags $(DEP_LIBS)` $(SNDFILE_CFLAGS) $(SAMPLERATE_CFLAGS) $(AM_CFLAGS)
emu4bm_CFLAGS = -I$(top_srcdir)/src `$(PKG_CONFIG) --cflags $(DEP_LIBS)` $(SNDFILE_CFLAGS) $(SAMPLERATE_CFLAGS) $(AM_CFLAGS)

emu3bm_LDFLAGS = `$(PKG_CONFIG) --libs $(DEP_LIBS)` $(SNDFILE_LIBS) $(SAMPLERATE_LIBS) -lm
emu4bm_LDFLAGS = `$(PKG_CONFIG) --libs $(DEP_LIBS)` $(SNDFILE_LIBS) $(SAMPLERATE_LIBS) -lm

bin_PROGRAMS = emu3bm emu4bm

emu3bm_SOURCES = main_emu3bm.c sfz.h emu3bm.c emu3bm.h sample.c sample.h utils.c utils.h
nodist_emu3bm_SOURCES = sfz.tab.c sfz.tab.h sfz.yy.c
BUILT_SOURCES = sfz.tab.c sfz.tab.h sfz.yy.c
EXTRA_DIST = sfz.y sfz.l

sfz.tab.c sfz.tab.h: sfz.y
	bison -Wcounterexamples -d sfz.y

sfz.yy.c: sfz.tab.h sfz.l
	flex -o sfz.yy.c sfz.l

emu4bm_SOURCES = main_emu4bm.c sample.c sample.h utils.c utils.h

SNDFILE_CFLAGS = @SNDFILE_CFLAGS@
SNDFILE_LIBS = @SNDFILE_LIBS@

SAMPLERATE_CFLAGS = @SAMPLERATE_CFLAGS@
SAMPLERATE_LIBS = @SAMPLERATE_LIBS@

CLEANFILES = sfz.tab.c sfz.tab.h sfz.yy.c
