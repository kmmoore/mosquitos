#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/module_manager.h>

#ifndef _MOSQUITOS
#define _MOSQUITOS
#define __mosquitos__
#endif

#define UNUSED __attribute__((unused))
#define WARN_UNUSED __attribute__((warn_unused_result))

#define STR(s) _STR(s)
#define _STR(s) #s

#endif
