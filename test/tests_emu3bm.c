#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "../src/emu3bm.h"

gfloat emu3_get_time_163_69_from_u8 (guint8 v);
guint8 emu3_get_u8_from_time_163_69 (gfloat v);

gint emu3_get_percent_from_s8 (const gint8 v);
gint8 emu3_get_s8_from_percent (gint v);

gint emu3_get_vcf_cutoff_frequency_from_u8 (const guint8 v);
guint8 emu3_get_u8_from_vcf_cutoff_frequency (const gint v);

gint emu3_get_percent_signed_from_s8 (gint8 v);
gint8 emu3_get_s8_from_percent_signed (gint v);

gint emu3_get_filter_id_from_sfz_fil_type (const gchar * sfz_fil_type);

gfloat emu3_get_vcf_tracking_from_s8 (const gint8 v);
gint8 emu3_get_s8_from_vcf_tracking (const gfloat v);

gfloat emu3_get_note_tuning_from_s8 (const gint8 v);
gint8 emu3_s8_from_get_note_tuning (const gfloat v);

static void
test_time_163_69 ()
{
  guint8 i;
  gfloat f;

  printf ("\n");

  f = emu3_get_time_163_69_from_u8 (100);
  i = emu3_get_u8_from_time_163_69 (f);
  CU_ASSERT_EQUAL (i, 100);

  CU_ASSERT_EQUAL (emu3_get_u8_from_time_163_69 (1000.0f), 127);
  CU_ASSERT_EQUAL (emu3_get_u8_from_time_163_69 (-1.0f), 0);

  CU_ASSERT_EQUAL (emu3_get_time_163_69_from_u8 (128), 163.69f);
}

static void
test_percent ()
{
  printf ("\n");

  CU_ASSERT_EQUAL (emu3_get_percent_from_s8 (-127), -100);
  CU_ASSERT_EQUAL (emu3_get_percent_from_s8 (0), 0);
  CU_ASSERT_EQUAL (emu3_get_percent_from_s8 (127), 100);

  CU_ASSERT_EQUAL (emu3_get_s8_from_percent (-100), -127);
  CU_ASSERT_EQUAL (emu3_get_s8_from_percent (0), 0);
  CU_ASSERT_EQUAL (emu3_get_s8_from_percent (100), 127);
}

void
test_vcf_cutoff_frequency ()
{
  guint8 i;
  gint cutoff;

  printf ("\n");

  cutoff = emu3_get_vcf_cutoff_frequency_from_u8 (100);
  i = emu3_get_u8_from_vcf_cutoff_frequency (cutoff);
  CU_ASSERT_EQUAL (i, 100);
}

static void
test_percent_signed ()
{
  printf ("\n");

  CU_ASSERT_EQUAL (emu3_get_percent_signed_from_s8 (-127), -100);
  CU_ASSERT_EQUAL (emu3_get_percent_signed_from_s8 (-1), -100);
  CU_ASSERT_EQUAL (emu3_get_percent_signed_from_s8 (0), -100);
  CU_ASSERT_EQUAL (emu3_get_percent_signed_from_s8 (0x40), 0);
  CU_ASSERT_EQUAL (emu3_get_percent_signed_from_s8 (127), 100);

  CU_ASSERT_EQUAL (emu3_get_s8_from_percent_signed (-200), 0);
  CU_ASSERT_EQUAL (emu3_get_s8_from_percent_signed (-100), 0);
  CU_ASSERT_EQUAL (emu3_get_s8_from_percent_signed (0), 0x40);
  CU_ASSERT_EQUAL (emu3_get_s8_from_percent_signed (100), 127);
  CU_ASSERT_EQUAL (emu3_get_s8_from_percent_signed (200), 127);
}

static void
test_emu3_get_filter_id_from_sfz_fil_type ()
{
  printf ("\n");

  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("apf_1p"), 11);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("bpf_2p"), 5);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("brf_2p"), 7);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("hpf_2p"), 3);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("lpf_2p"), 0);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("hpf_4p"), 4);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("lpf_4p"), 1);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("lpf_6p"), 2);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("comb"), 14);
  CU_ASSERT_EQUAL (emu3_get_filter_id_from_sfz_fil_type ("foo"), 0);
}

static void
test_vcf_tracking ()
{
  printf ("\n");

  CU_ASSERT_EQUAL (emu3_get_vcf_tracking_from_s8 (-127), -2);
  CU_ASSERT_EQUAL (emu3_get_vcf_tracking_from_s8 (127), 2);
  CU_ASSERT_EQUAL (emu3_get_vcf_tracking_from_s8 (64), 1);
  CU_ASSERT_EQUAL (emu3_get_vcf_tracking_from_s8 (-64), -1);
  CU_ASSERT_EQUAL (emu3_get_vcf_tracking_from_s8 (0), 0);

  CU_ASSERT_EQUAL (emu3_get_s8_from_vcf_tracking (-2), -127);
  CU_ASSERT_EQUAL (emu3_get_s8_from_vcf_tracking (2), 127);
  CU_ASSERT_EQUAL (emu3_get_s8_from_vcf_tracking (1), 64);
  CU_ASSERT_EQUAL (emu3_get_s8_from_vcf_tracking (-1), -64);
  CU_ASSERT_EQUAL (emu3_get_s8_from_vcf_tracking (0), 0);
}

static void
test_note_tuning ()
{
  printf ("\n");

  CU_ASSERT_EQUAL (emu3_get_note_tuning_from_s8 (-64), -100);
  CU_ASSERT_EQUAL (emu3_get_note_tuning_from_s8 (0), 0);
  CU_ASSERT_EQUAL (emu3_get_note_tuning_from_s8 (64), 100);

  CU_ASSERT_EQUAL (emu3_s8_from_get_note_tuning (-100), -64);
  CU_ASSERT_EQUAL (emu3_s8_from_get_note_tuning (0), 0);
  CU_ASSERT_EQUAL (emu3_s8_from_get_note_tuning (100), 64);
}

gint
main (gint argc, gchar *argv[])
{
  gint err = 0;

  verbosity = 5;

  if (CU_initialize_registry () != CUE_SUCCESS)
    {
      goto cleanup;
    }
  CU_pSuite suite = CU_add_suite ("emu3bm tests", 0, 0);
  if (!suite)
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "time_163_69", test_time_163_69))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "percent", test_percent))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "vcf_cutoff_frequency", test_vcf_cutoff_frequency))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "percent_signed", test_percent_signed))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "emu3_get_filter_id_from_sfz_fil_type",
		    test_emu3_get_filter_id_from_sfz_fil_type))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "vcf_tracking", test_vcf_tracking))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "note_tuning", test_note_tuning))
    {
      goto cleanup;
    }

  CU_basic_set_mode (CU_BRM_VERBOSE);

  CU_basic_run_tests ();
  err = CU_get_number_of_tests_failed ();

cleanup:
  CU_cleanup_registry ();
  return err || CU_get_error ();
}
