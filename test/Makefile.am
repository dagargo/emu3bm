check_PROGRAMS = tests_emu3bm

AM_CPPFLAGS = -DDATADIR='"$(datadir)"'

tests_LIBS = glib-2.0 cunit

tests_emu3bm_CFLAGS = -I$(top_srcdir)/src `$(PKG_CONFIG) --cflags $(tests_LIBS)` $(SNDFILE_CFLAGS) $(SAMPLERATE_CFLAGS) $(AM_CFLAGS)
tests_emu3bm_LDFLAGS = `$(PKG_CONFIG) --libs $(tests_LIBS)` $(SNDFILE_LIBS) $(SAMPLERATE_LIBS) -lm

tests_emu3bm_SOURCES = \
	tests_emu3bm.c \
	../src/emu3bm.c \
	../src/emu3bm.h \
	../src/sample.c \
	../src/sample.h \
	../src/sfz.tab.c \
	../src/sfz.tab.h \
	../src/sfz.yy.c \
	../src/sfz.h \
	../src/utils.c \
	../src/utils.h

TESTS = $(check_PROGRAMS) \
	emu3_test_add_preset.sh \
	emu3_test_add_sample.sh \
	emu3_test_add_sfz.sh \
	emu3_test_add_zone.sh \
	emu3_test_create_bank.sh \
	emu3_test_edit_parameter.sh \
	emu3_test_extract_samples.sh \
	emu4_test_add_sample.sh \
	emu4_test_create_bank.sh \
	emu4_test_extract_samples.sh
