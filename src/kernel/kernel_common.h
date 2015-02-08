#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <kernel/module_manager.h>

#ifndef _MOSQUITOS
#define _MOSQUITOS
#define __mosquitos__
#endif

#define UNUSED __attribute__((unused))

#define STR(s) _STR(s)
#define _STR(s) #s

#endif