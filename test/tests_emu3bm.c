#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "../src/emu3bm.h"

gfloat emu3_get_time_163_69_from_u8 (guint8 v);
guint8 emu3_get_u8_from_time_163_69 (gfloat v);

gint emu3_get_percent_s8 (const gint8 v);
gint8 emu3_get_s8_from_percent (gint v);

gint emu3_get_vcf_cutoff_frequency_from_u8 (const guint8 v);
guint8 emu3_get_u8_from_vcf_cutoff_frequency (const gint v);

gint emu3_get_percent_signed_from_s8 (gint8 v);
gint8 emu3_get_s8_from_percent_signed (gint v);

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

  CU_ASSERT_EQUAL (emu3_get_percent_s8 (-127), -100);
  CU_ASSERT_EQUAL (emu3_get_percent_s8 (0), 0);
  CU_ASSERT_EQUAL (emu3_get_percent_s8 (127), 100);

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

  CU_basic_set_mode (CU_BRM_VERBOSE);

  CU_basic_run_tests ();
  err = CU_get_number_of_tests_failed ();

cleanup:
  CU_cleanup_registry ();
  return err || CU_get_error ();
}
