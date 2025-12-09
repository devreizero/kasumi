#pragma once
#ifndef ASSERT_H
#define ASSERT_H

#include <panic.h>

#ifndef NDEBUG

#define assert(cond)                                                           \
  do {                                                                         \
    if (!(cond))                                                               \
      panic(#cond);                                                            \
  } while (0)

#define assert2(cond, ...)                                                     \
  do {                                                                         \
    if (!(cond))                                                               \
      panic(#cond ": " __VA_ARGS__);                                           \
  } while (0)

#else

#define assert(cond)
#define assert2(cond, message) ;

#endif

#endif