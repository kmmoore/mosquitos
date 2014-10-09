#include "kernel_common.h"

#ifndef _MODULE_MANAGER_H
#define _MODULE_MANAGER_H

void module_manager_init();
void module_manager_set_initialized(const char *module_name);
bool module_manager_module_is_initialized(const char *module_name);

#endif