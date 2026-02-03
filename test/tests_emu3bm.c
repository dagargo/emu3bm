#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "../src/emu3bm.h"

gfloat emu3_get_time_163_69_from_u8 (uint8_t v);
uint8_t emu3_get_u8_from_time_163_69 (gfloat v);

int emu3_get_percent_s8 (const int8_t v);
int8_t emu3_get_s8_from_percent (int v);

void
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

void
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

  if (!CU_add_test (suite, "test_time_163_69", test_time_163_69))
    {
      goto cleanup;
    }

  if (!CU_add_test (suite, "test_percent", test_percent))
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
