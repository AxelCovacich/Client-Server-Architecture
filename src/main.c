// src/main.c

#include "dummy.h"
#include <stdio.h>
/**
 * @brief Main entry of the application.
 *
 * Calls run_dummy_test() from CoreLib.
 *
 * @return Return code from run_dummy_test().
 */

int main(void) {
  printf("Starting LogisticsServer...\n");

  int result = run_dummy_test();
  if (result != 0) {
    fprintf(stderr, "Error: run_dummy_test() returned %d\n", result);
    return result;
  }

  printf("run_dummy_test() completed successfully.\n");

  /* TODO: Initialize and run the actual loop here */

  return 0;
}
