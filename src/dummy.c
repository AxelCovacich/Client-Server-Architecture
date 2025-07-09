// src/dummy.c

#include "dummy.h" // pull in the prototype
#include <stdio.h>

/**
 * @brief Runs the dummy build test.
 *
 * Prints a confirmation message to stdout.
 *
 * @return 0 on success, non-zero on failure.
 */
int run_dummy_test(void) {
  printf("Dummy file running OK\n");
  return 0;
}
