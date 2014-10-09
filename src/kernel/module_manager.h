#ifndef _MODULE_MANAGER_H
#define _MODULE_MANAGER_H

#include "kernel_common.h"
#include "util.h"

#ifdef DEBUG
  #define REGISTER_MODULE(module_name) module_manager_set_initialized(module_name)
  #define REQUIRE_MODULE(module_name) assert(module_manager_is_initialized(module_name)) // NOTE: This doesn't work before interrupts are enabled
#else
  // Only check runtime module dependencies in debug builds
  #define REGISTER_MODULE(module_name)
  #define REQUIRE_MODULE(module_name)
#endif

void module_manager_init();
void module_manager_set_initialized(const char *module_name);
bool module_manager_is_initialized(const char *module_name);

#endif