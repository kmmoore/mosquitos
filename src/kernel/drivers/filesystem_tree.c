#include <kernel/drivers/filesystem_tree.h>
#include <kernel/drivers/filesystems/mfs.h>
#include <kernel/drivers/text_output.h>

#define MAX_FILESYSTEMS 10

static struct FilesystemTreeData {
  Filesystem filesystems[MAX_FILESYSTEMS];
  size_t num_filesystems;
} fs_tree_data;

static FilesystemError add_filesystem(const char *const filesystem_id,
                                      const void *initialization_data) {
  assert(fs_tree_data.num_filesystems < MAX_FILESYSTEMS);
  Filesystem *new_filesystem =
      &fs_tree_data.filesystems[fs_tree_data.num_filesystems++];
  if (!filesystem_create(filesystem_id, new_filesystem)) {
    return FS_ERROR_NOT_FOUND;
  }

  text_output_printf("Adding filesystem to tree: %s at index %i\n",
                     filesystem_id, fs_tree_data.num_filesystems - 1);

  return new_filesystem->init(new_filesystem, initialization_data);
}

void filesystem_tree_init() {
  REQUIRE_MODULE("ahci");
  REQUIRE_MODULE("pci");

  fs_tree_data.num_filesystems = 0;

  PCIDevice *device = pci_find_device(0x01, 0x06, 0x01);
  if (device == NULL || !device->has_driver) {
    return;
  }

  PCIDeviceDriver *driver = &device->driver;
  uint64_t num_devices = -1;
  PCIDeviceDriverError ahci_error =
      driver->execute_command(driver, AHCI_COMMAND_NUM_DEVICES, NULL, 0,
                              &num_devices, sizeof(num_devices));
  if (ahci_error != PCI_ERROR_NONE) return;

  for (uint64_t i = 0; i < num_devices; ++i) {
    const struct AHCIDeviceInfoCommand info_request = {.device_id = i};
    struct AHCIDeviceInfo info;
    ahci_error =
        driver->execute_command(driver, AHCI_COMMAND_DEVICE_INFO, &info_request,
                                sizeof(info_request), &info, sizeof(info));
    assert(ahci_error == PCI_ERROR_NONE);

    if (info.device_type == AHCI_DEVICE_SATA) {
      const MFSSATAInitData initialization_data = {.driver = driver,
                                                   .device_id = i};
      // TODO: Discover what type of FS is on the device and load that driver,
      // instead of always loading MFS_S.
      add_filesystem("MFS_S", &initialization_data);
    }
  }
}

FilesystemError filesystem_tree_add(const char *const identifier,
                                    const void *initialization_data,
                                    Filesystem **filesystem) {
  *filesystem = &fs_tree_data.filesystems[fs_tree_data.num_filesystems];
  return add_filesystem(identifier, initialization_data);
}

Filesystem *filesystem_tree_get(size_t index) {
  if (index >= fs_tree_data.num_filesystems) {
    return NULL;
  }

  return &fs_tree_data.filesystems[index];
}

size_t filesystem_tree_size() { return fs_tree_data.num_filesystems; }
