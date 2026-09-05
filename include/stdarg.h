#ifndef CC_STDARG_H
#define CC_STDARG_H

#if defined(__powerpc__) && defined(__ELF__)
/* The System V register cursor and its two pointers occupy twelve bytes. */
typedef unsigned int va_list[3];
#define __CC_VA_ADDRESS(ap) (ap)
#else
typedef char *va_list;
#define __CC_VA_ADDRESS(ap) (&(ap))
#endif

#define va_start(ap, last) __builtin_va_start(__CC_VA_ADDRESS(ap), &(last))
#define va_arg(ap, type) __builtin_va_arg(__CC_VA_ADDRESS(ap), type)
#define va_copy(destination, source)                                                               \
	__builtin_va_copy(__CC_VA_ADDRESS(destination), __CC_VA_ADDRESS(source))
#define va_end(ap) __builtin_va_end(__CC_VA_ADDRESS(ap))

#endif
