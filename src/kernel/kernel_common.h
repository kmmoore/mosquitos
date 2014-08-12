#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _MOSQUITOS
#define _MOSQUITOS
#define __mosquitos__
#endif

#define UNUSED __attribute__((unused))

#define STR(s) _STR(s)
#define _STR(s) #s