#ifndef TEST_UTIL_H
#define TEST_UTIL_H

/* Minimal runner shared by the C unit tests.
 *
 * A failing assert() aborts the program, so reaching TEST_SUMMARY means every
 * test function passed. The summary matches the shell runners' format:
 * "<suite>: <passed>/<total> passed".
 */

#include <stdio.h>

static int test_count = 0;

#define RUN_TEST(test)                                                         \
  do {                                                                         \
    (test)();                                                                  \
    test_count++;                                                              \
  } while (0)

#define TEST_SUMMARY(suite)                                                    \
  do {                                                                         \
    printf("%s: %d/%d passed\n", (suite), test_count, test_count);             \
    return 0;                                                                  \
  } while (0)

#endif
