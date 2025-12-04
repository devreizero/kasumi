#pragma once
#ifndef ASSERT_H
#define ASSERT_H

#include <basic-io.h>
#include <format_string.h>
#include <serial.h>

#ifndef NDEBUG

static inline void __assert_fail(const char filename[], const unsigned int lineno, const char *message) {
  char line[5];
  formatInteger(lineno, line, 5, OPTION_NONNULL | OPTION_NUM_PADZERO);

  serialPuts(filename);
  serialPutc(':');
  serialPutsn(line, 5);
  serialPuts(": ");
  serialPuts(message);
  serialPuts("\r\n");

  hang();
}

#define assert(cond)                                                           \
  do {                                                                         \
    if (!(cond))                                                               \
      __assert_fail(__FILE_NAME__, __LINE__, #cond);                                                    \
  } while (0)

#define assert2(cond, message)                                                 \
  do {                                                                         \
    if (!(cond))                                                               \
      __assert_fail(__FILE_NAME__, __LINE__, #cond ": " message);                                       \
  } while (0)

#else

static inline void __assert_fail(const char *message) {}
#define assert(cond)
#define assert2(cond, message) ;

#endif

#endif