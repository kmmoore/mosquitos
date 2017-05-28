#include <kernel/kernel_common.h>

#ifndef _FILESYSTEM_INTERFACE_H
#define _FILESYSTEM_INTERFACE_H

#define FILESYSTEM_BLOCK_SIZE_BYTES 512
#define FILESYSTEM_MAX_DIR_NAME_LENGTH 247

typedef enum _FilesystemError FilesystemError;
typedef struct _Directory Directory;
typedef struct _File File;
typedef struct _Filesystem Filesystem;
typedef struct _DirectoryEntry DirectoryEntry;
typedef struct _FilesystemBlock FilesystemBlock;
typedef struct _FilesystemInfo FilesystemInfo;

enum _FilesystemError {
  FS_ERROR_NONE = 0,
  FS_ERROR_COMMAND_NOT_IMPLEMENTED,
  FS_ERROR_INVALID_PARAMETERS,
  FS_ERROR_NOT_FOUND,
  FS_ERROR_INPUT_BUFFER_TOO_SMALL,
  FS_ERROR_OUTPUT_BUFFER_TOO_SMALL,
  FS_ERROR_DEVICE_ERROR,
  FS_ERROR_OUT_OF_SPACE,
  FS_ERROR_CORRUPT_FILESYSTEM
};

struct _DirectoryEntry {
  uint64_t id;
  char name[FILESYSTEM_MAX_DIR_NAME_LENGTH + 1];  // Null terminated
};

struct _FilesystemBlock {
  uint8_t data[FILESYSTEM_BLOCK_SIZE_BYTES];
};

struct _FilesystemInfo {
  bool formatted;
  uint64_t size_blocks;
  char volume_name[256];
};

typedef FilesystemError (*FilesystemReadBlocks)(Filesystem *filesystem,
                                                uint64_t block_start,
                                                uint64_t num_blocks,
                                                void *blocks);
typedef FilesystemError (*FilesystemWriteBlocks)(Filesystem *filesystem,
                                                 uint64_t block_start,
                                                 uint64_t num_blocks,
                                                 const void *blocks);

typedef FilesystemError (*FilesystemInitFunction)(Filesystem *filesystem,
                                                  void *initialization_data);
typedef FilesystemError (*FilesystemFormatFunction)(
    Filesystem *filesystem, const char const *volume_name,
    uint64_t size_blocks);
typedef FilesystemError (*FilesystemInfoFunction)(Filesystem *filesystem,
                                                  FilesystemInfo *info);

typedef FilesystemError (*FilesystemCreateDirectoryFunction)(
    Filesystem *filesystem, const char const *name,
    const Directory *parent_directory, Directory **directory);
typedef FilesystemError (*FilesystemOpenDirectoryFunction)(
    Filesystem *filesystem, const char const *path, Directory **directory);
typedef FilesystemError (*FilesystemCloseDirectoryFunction)(
    Filesystem *filesystem, Directory *directory);
typedef FilesystemError (*FilesystemReadDirectoryEntryFunction)(
    Filesystem *filesystem, Directory *const directory, DirectoryEntry *entry);
typedef FilesystemError (*FilesystemSeekFrontDirectoryFunction)(
    Filesystem *filesystem, Directory *const directory);

typedef FilesystemError (*FilesystemCreateFileFunction)(
    Filesystem *filesystem, const Directory *parent_directory,
    const char const *name, File **file);
typedef FilesystemError (*FilesystemOpenFileFunction)(Filesystem *filesystem,
                                                      const char const *path,
                                                      File **file);
typedef FilesystemError (*FilesystemCloseFileFunction)(Filesystem *filesystem,
                                                       File *file);
typedef FilesystemError (*FilesystemReadFileFunction)(Filesystem *filesystem,
                                                      File *file,
                                                      uint64_t *length,
                                                      uint8_t *buffer);
typedef FilesystemError (*FilesystemWriteFileFunction)(Filesystem *filesystem,
                                                       File *file,
                                                       const uint8_t *buffer,
                                                       const uint64_t length);
typedef FilesystemError (*FilesystemSeekFileFunction)(Filesystem *filesystem,
                                                      File *file,
                                                      const uint64_t position);
typedef FilesystemError (*FilesystemTellFileFunction)(Filesystem *filesystem,
                                                      const File *file,
                                                      uint64_t *position);

struct _Filesystem {
  const char *identifier;
  const char *long_name;
  void *data;

  FilesystemReadBlocks read_blocks;
  FilesystemWriteBlocks write_blocks;

  FilesystemInitFunction init;
  FilesystemFormatFunction format;
  FilesystemInfoFunction info;

  FilesystemCreateDirectoryFunction create_directory;
  FilesystemOpenDirectoryFunction open_directory;
  FilesystemCloseDirectoryFunction close_directory;
  FilesystemReadDirectoryEntryFunction read_directory_entry;
  FilesystemSeekFrontDirectoryFunction seek_front_directory;

  FilesystemCreateFileFunction create_file;
  FilesystemOpenFileFunction open_file;
  FilesystemCloseFileFunction close_file;
  FilesystemReadFileFunction read_file;
  FilesystemWriteFileFunction write_file;
  FilesystemSeekFileFunction seek_file;
  FilesystemTellFileFunction tell_file;
};

#endif
