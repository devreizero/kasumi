// AI SLOP because I don't bother making myself a printf

#include <serial.h>
#include <stdarg.h>
#include <stddef.h>

static void reverse(char *str, size_t len) {
  size_t i = 0, j = len - 1;
  while (i < j) {
    char tmp = str[i];
    str[i] = str[j];
    str[j] = tmp;
    i++;
    j--;
  }
}

static size_t ulong_to_str(unsigned long val, char *buf, int base) {
  const char *digits = "0123456789abcdef";
  size_t i = 0;
  if (val == 0) {
    buf[i++] = '0';
    return i;
  }
  while (val) {
    buf[i++] = digits[val % base];
    val /= base;
  }
  reverse(buf, i);
  return i;
}

int vsprintf(char *out, const char *fmt, va_list args) {
  char *buf = out;
  while (*fmt) {
    if (*fmt != '%') {
      if (*fmt == '\n')
        *buf++ = '\r';
      *buf++ = *fmt++;
      continue;
    }
    fmt++; // skip '%'

    // Width for %*s
    int width = 0;
    if (*fmt == '*') {
      width = va_arg(args, int);
      fmt++;
    }

    switch (*fmt) {
    case 's': {
      const char *s = va_arg(args, const char *);
      int len = 0;
      while (s[len])
        len++;
      int pad = width - len;
      while (pad-- > 0)
        *buf++ = ' ';
      while (*s)
        *buf++ = *s++;
      break;
    }
    case 'c': {
      char c = (char)va_arg(args, int);
      *buf++ = c;
      break;
    }
    case 'l': {
      fmt++;
      unsigned long val = va_arg(args, unsigned long);
      char tmp[32];
      size_t len = 0;
      if (*fmt == 'u') {
        len = ulong_to_str(val, tmp, 10);
      } else if (*fmt == 'x') {
        len = ulong_to_str(val, tmp, 16);
      }
      for (size_t i = 0; i < len; i++)
        *buf++ = tmp[i];
      break;
    }
    case 'x': {
      unsigned int val = va_arg(args, unsigned int);
      char tmp[32];
      size_t len = ulong_to_str(val, tmp, 16);
      for (size_t i = 0; i < len; i++)
        *buf++ = tmp[i];
      break;
    }
    default:
      *buf++ = *fmt;
      break;
    }
    fmt++;
  }
  *buf = '\0';
  return buf - out;
}

int printf(const char *fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(buf, fmt, args);
  va_end(args);

  serialPutsn(buf, len);
  return len;
}
