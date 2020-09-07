#ifndef @(name.upper())_H_GENERATED_
#define @(name.upper())_H_GENERATED_

#if !LELY_NO_MALLOC
#error Static object dictionaries are only supported when dynamic memory allocation is disabled.
#endif

#include <lely/co/co.h>

#ifdef __cplusplus
extern "C" {
#endif

co_dev_t * @(name)_init(void);

#ifdef __cplusplus
}
#endif

#endif // @(name.upper())_H_GENERATED_
