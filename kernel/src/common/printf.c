// AI SLOP because I don't bother making myself a printf

#include <serial.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

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

static unsigned long strToUlong(const char *s, int len) {
  unsigned long value = 0;

  for (int i = 0; i < len; i++) {
      value = value * 10 + (s[i] - '0');
  }

  return value;
}

int vsprintf(char *out, const char *fmt, va_list args) {
  unsigned long val;
  char *buf = out;
  int width;
  uint8_t base, useLong;
  while (*fmt) {
    if (*fmt != '%') {
      if (*fmt == '\n')
        *buf++ = '\r';
      *buf++ = *fmt++;
      continue;
    }
    fmt++; // skip '%'

    // Width for %*s
    width = 0;
    if (*fmt == '*') {
      width = va_arg(args, int);
      fmt++;
    } else if (*fmt >= '0' && *fmt <= '9') {
      char tmp[32];
      int i = 0;
      do {
        tmp[i++] = *fmt++;
      } while(*fmt >= '0' && *fmt <= '9' && i < 32);
      width = strToUlong(tmp, i);
    }

    useLong = 0;
    base = 10;
    start: switch (*fmt) {
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
      goto endIncremented;
    }
    case 'c': {
      char c = (char)va_arg(args, int);
      *buf++ = c;
      goto endIncremented;
    }
    case 'l': {
      fmt++;
      useLong = 1;
      goto start;
    }
    case 'p': {
      uintptr_t val = va_arg(args, uintptr_t);
      if(val == 0) {
        *buf++ = 'n';
        *buf++ = 'u';
        *buf++ = 'l';
        *buf++ = 'l';
        goto endIncremented;
      } else {
        *buf++ = '0';
        *buf++ = 'x';
        fmt++;
        goto number;
      }
    }
    case 'z': {
      if (*++fmt != 'u') {
        *buf++ = '%';
        *buf++ = 'z';
        *buf++ = *fmt;
        goto end;
      }

      val = va_arg(args, size_t);
      goto format;
    }
    case 'x': base = 16; goto number;
    case 'u': base = 10; goto number;
    default:
      if (*fmt != '%') *buf++ = '%';
      *buf++ = *fmt;
      goto endIncremented;
    }

    number:
      val = useLong ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
    
    format: { 
      char tmp[32];
      size_t len = ulong_to_str(val, tmp, base);
      
      int pad = width - len;
      while (pad-- > 0)
        *buf++ = ' ';
      
      for (size_t i = 0; i < len; i++)
        *buf++ = tmp[i];
    }

    endIncremented:
    fmt++;

    end: {}
  }

  *buf = '\0';
  return buf - out;
}

int fprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(out, fmt, args);
  va_end(args);

  return len;
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
