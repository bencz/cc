#ifndef CC_STDARG_H
#define CC_STDARG_H

typedef char *va_list;

#define __CC_VA_SIZE(type) ((sizeof(type) + 3U) & ~3U)
#define va_start(ap, last) ((ap) = (char *)&(last) + __CC_VA_SIZE(last))
#define va_arg(ap, type) (*(type *)((ap) += __CC_VA_SIZE(type), (ap) - __CC_VA_SIZE(type)))
#define va_copy(destination, source) ((destination) = (source))
#define va_end(ap) ((void)(ap))

#endif
