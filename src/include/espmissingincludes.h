#ifndef ESPMISSINGINCLUDES_H
#define ESPMISSINGINCLUDES_H

#include <stdint.h>
#include <c_types.h>
#include <stdarg.h>
#include <os_type.h>
#include <mem.h>
#include <user_interface.h>

int ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
int ets_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
int ets_vsnprintf(char *str, size_t size, const char *format, va_list ap);

#ifndef malloc
#define malloc os_malloc
#endif

#ifndef zalloc
#define zalloc os_zalloc
#endif

#ifndef free
#define free os_free
#endif

#ifndef realloc
#define realloc os_realloc
#endif

#ifndef vsnprintf
#define vsnprintf ets_vsnprintf
#endif

#endif
