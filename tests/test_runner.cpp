#include "unity.h"

extern "C" {

/**
 * @brief Unity hook called before each test.
 */
void setUp(void) {
}

/**
 * @brief Unity hook called after each test.
 */
void tearDown(void) {
}

/* Prototypes for C tests */
// void test_make_leak_detected(void);

void test_dummy_always_passes(void);
} // extern "C"

/**
 * @brief Runs all the tests.
 *
 * Call all the tests for each c and cpp files in one runner(this file)
 *
 * @return 0 on success, non-zero on failure.
 */
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_dummy_always_passes);
  // RUN_TEST(test_make_leak_detected);
  // add more tests here;
  return UNITY_END();
}