#ifndef ESPMISSINGINCLUDES_H
#define ESPMISSINGINCLUDES_H

#include <c_types.h>
#include <mem.h>
#include <os_type.h>
#include <osapi.h>
#include <stdarg.h>
#include <stdint.h>
#include <user_interface.h>

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
