#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "coap_debug.h"

void coap_debug_error(char *format, ...) {
  static char timebuf[32];
  struct tm *tmp;
  time_t now;
  va_list ap;

  time(&now);
  tmp = localtime(&now);

  if ( strftime(timebuf,sizeof(timebuf), "%b %d %H:%M:%S", tmp) )
    printf("%s ", timebuf);

  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  fflush(stdout);
}
