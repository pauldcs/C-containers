#include "array.h"
#include "unit_tests.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static void bubble_sort(array_t *array) {
  size_t size = array_size(array);

  while (true) {
    bool sorted = true;
    size_t i = 0;
    while (i + 1 < size) {
      if (*(int *)array_at(array, i) > *(int *)array_at(array, i + 1)) {
        array_swap(array, i, i + 1);
        sorted = false;
      }
      i++;
    }
    if (sorted)
      break;
  }
}

static int *generate_random_numbers(void) {
  static int randomNumbers[10000];
  srand(time(NULL));
  for (int i = 0; i < 10000; ++i) {
    randomNumbers[i] = rand() % 10000;
  }
  return randomNumbers;
}

static bool __test_001__(void) {
  array_t *arr = array_create(sizeof(int), 0, NULL);
  int *randnums = generate_random_numbers();
  size_t i = 10000;
  while (i--)
    array_push(arr, &randnums[i]);

  bubble_sort(arr);
  //array_stats(arr);

  size_t j = 0;
  while (j + 1 < 10000) {
    if (*(int *)array_at(arr, j) > *(int *)array_at(arr, j + 1)) {
      return (false);
    }
    j++;
  }
  return (true);
}

TEST_FUNCTION void array_usage_specs(void) {
  __test_start__;

  run_test(&__test_001__, "push 10000 1 by 1 and bubble sort them");

  __test_end__;
}
