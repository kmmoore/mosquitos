#include "module_manager.h"
#include "../common/mem_util.h"

#define kMaxModules 32
#define kModuleNameMaxLength 24

static struct {
  size_t num_modules;
  struct {
    char name[kModuleNameMaxLength + 1];
  } modules[kMaxModules];
} module_manager_data;


void module_manager_init() {
  module_manager_data.num_modules = 0;
}

void module_manager_set_initialized(const char *module_name) {
  assert(module_manager_data.num_modules < kMaxModules);

  char *new_module = module_manager_data.modules[module_manager_data.num_modules++].name;
  size_t name_len = strlcpy(new_module, module_name, kModuleNameMaxLength + 1);
  assert(name_len < kModuleNameMaxLength + 1);
}

bool module_manager_is_initialized(const char *module_name) {
  for (size_t i = 0; i < module_manager_data.num_modules; ++i) {
    if (strcmp(module_name, module_manager_data.modules[i].name) == 0) return true;
  }
  return false;
}
