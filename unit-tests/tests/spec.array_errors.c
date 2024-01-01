
#include "array.h"
#include "internal.h"
#include "unit_tests.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static bool __test_001__(void) {
  ASSERT_IS_NULL((void *)array_create(0, 16, NULL));
  ASSERT_IS_NULL((void *)array_create(0, 0, NULL));
  ASSERT_IS_NULL((void *)array_create(SIZE_TYPE_MAX / 2, 4, NULL));
  ASSERT_IS_NULL((void *)array_create(4, SIZE_TYPE_MAX / 2, NULL));
  return (true);
}

static bool __test_002__(void) {
  array_t *a = array_create(sizeof(int), 16, NULL);

  int __arr__[] = {111, 222, 333, 444, 555, 666, 777, 888, 999};

  ASSERT_NUM_EQUAL(array_inject(a, 1, __arr__, 3), false, "%d");
  ASSERT_IS_NULL((void *)array_at(a, 0));

  array_push(a, __PTRIZE_ST32__(1));
  array_push(a, __PTRIZE_ST32__(-1));

  ASSERT_NUM_EQUAL(array_inject(a, 0, __arr__, 0), true, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 3, __arr__, 1), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 0, __arr__, 0), true, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 3, __arr__, 1), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 4, __arr__, 1), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 1, __arr__, 9), true, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 2, &__arr__[8], 1), true, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, array_size(a) + 1, "", 1), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 1000, __arr__, 9), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 1, __arr__, SIZE_TYPE_MAX / 4), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, SIZE_T_MAX, __arr__, 0), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(a, 5, NULL, 5), false, "%d");
  ASSERT_NUM_EQUAL(array_inject(NULL, 5, __arr__, 5), false, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_head(a), 1, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_at(a, 0), 1, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_tail(a), -1, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_at(a, 2), 999, "%d");
  return (true);
}

static bool __test_003__(void) {
  array_t *a = array_create(sizeof(int), 16, NULL);

  int __arr__[] = {111, 222, 333, 444, 555, 666, 777, 888, 999};

  ASSERT_NUM_EQUAL(array_append(a, __arr__, 2), true, "%d");
  ASSERT_NUM_EQUAL(array_append(a, __arr__, 0), true, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_at(a, 0), 111, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_at(a, 1), 222, "%d");
  ASSERT_IS_NULL((void *)array_at(a, 2));
  ASSERT_NUM_EQUAL(array_append(a, NULL, 2), false, "%d");
  ASSERT_NUM_EQUAL(array_append(NULL, __arr__, 2), false, "%d");
  ASSERT_NUM_EQUAL(array_append(a, __arr__, SIZE_T_MAX), false, "%d");
  ASSERT_NUM_EQUAL(array_append(a, __arr__, 9), true, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_at(a, array_size(a) - 1), 999, "%d");
  ASSERT_NUM_EQUAL(*(int *)array_at(a, 0), 111, "%d");
  array_trace(a);
  return (true);
}
TEST_FUNCTION void array_error_specs(void) {
  __test_start__;

  run_test(&__test_001__, "array create");
  run_test(&__test_002__, "array inject");
  run_test(&__test_003__, "array append");

  __test_end__;
}
