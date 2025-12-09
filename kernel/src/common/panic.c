#include <basic_io.h>
#include <panic.h>
#include <printf.h>
#include <serial.h>
#include <stdarg.h>

__noreturn void __panicInternal(const char *filename, unsigned int line,
                                const char *fmt, ...) {
  char buf[1024];
  int len1 = fprintf(buf, "assert: ");

  va_list args;
  va_start(args, fmt);
  int len2 = vsprintf(buf, fmt, args);
  va_end(args);

  int len3 = fprintf(buf, "\n>>> at %s:%u\n", filename, line);
  int len  = len1 + len2 + len3;

  serialPutsn(buf, len);
  hang();
}