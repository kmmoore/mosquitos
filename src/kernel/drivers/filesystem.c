#include <common/mem_util.h>
#include <kernel/drivers/filesystem.h>
#include <kernel/drivers/text_output.h>

#define FILESYSTEM_MAX_FILESYSTEMS 20

static struct {
  Filesystem filesystem_templates[FILESYSTEM_MAX_FILESYSTEMS];
  int num_filesystems;
} filesystem_data;

void filesystem_init() { REGISTER_MODULE("filesystem"); }

void filesystem_register(Filesystem filesystem) {
  assert(filesystem_data.num_filesystems < FILESYSTEM_MAX_FILESYSTEMS);
  text_output_printf("Registering filesystem: %s (%s)\n", filesystem.long_name,
                     filesystem.identifier);
  filesystem_data.filesystem_templates[filesystem_data.num_filesystems++] =
      filesystem;
}

bool filesystem_create(const char *const identifier, Filesystem *filesystem) {
  for (int i = 0; i < filesystem_data.num_filesystems; ++i) {
    if (strcmp(filesystem_data.filesystem_templates[i].identifier,
               identifier) == 0) {
      *filesystem = filesystem_data.filesystem_templates[i];
      return true;
    }
  }

  return false;
}

const char *filesystem_error_string(FilesystemError error) {
  switch (error) {
    case FS_ERROR_NONE:
      return "No error";
    case FS_ERROR_COMMAND_NOT_IMPLEMENTED:
      return "Command not implemented";
    case FS_ERROR_INVALID_PARAMETERS:
      return "Invalid parameters";
    case FS_ERROR_NOT_FOUND:
      return "Not found";
    case FS_ERROR_INPUT_BUFFER_TOO_SMALL:
      return "Input buffer too small";
    case FS_ERROR_OUTPUT_BUFFER_TOO_SMALL:
      return "Output buffer too small";
    case FS_ERROR_DEVICE_ERROR:
      return "Device error";
    case FS_ERROR_OUT_OF_SPACE:
      return "Out of space";
    default:
      return "Unknown error code";
  }
}
