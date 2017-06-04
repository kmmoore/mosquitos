#include <common/math.h>
#include <common/mem_util.h>
#include <kernel/datastructures/list.h>
#include <kernel/drivers/filesystems/mfs.h>
#include <kernel/drivers/pci_drivers/ahci/ahci.h>
#include <kernel/drivers/text_output.h>
#include <kernel/memory/kmalloc.h>

#define MFS_MAGIC_STRING "MOSQUIFS"
#define BITMAP_ENTRIES_PER_BLOCK (NUM_BITS(FILESYSTEM_BLOCK_SIZE_BYTES))
#define BITMAP_ENTRIES_PER_WORD (NUM_BITS(sizeof(uint64_t)))
#define BLOCK_NUMBERS_PER_INDIRECT_BLOCK \
  (FILESYSTEM_BLOCK_SIZE_BYTES / sizeof(uint64_t))
#define DIRECTORY_ENTRIES_PER_BLOCK \
  (FILESYSTEM_BLOCK_SIZE_BYTES / sizeof(DirectoryEntry))
#define ROOT_DIRECTORY_INODE 1

enum InodeFlags {
  INODE_FLAG_NONE = 0,
  INODE_FLAG_DIRECTORY = 1 << 0,
};

typedef struct {
  bool formatted;
  uint64_t size_blocks;
  char volume_name[256];
  uint64_t num_data_blocks;
  uint64_t num_inodes;
  uint64_t uuid1;
  uint64_t uuid2;

  List open_inodes;

  union {
    struct {
      PCIDeviceDriver *driver;
      int device_id;
      uint32_t sector_size;
    } sata;

    struct {
      FilesystemBlock *blocks;
    } in_memory;
  } device;
} MFSData;

// TODO: Make this endien-independent
// Note: sizeof(MetadataBlock) must be <= 512 bytes
typedef struct {
  char magic[8];
  uint64_t size_blocks;
  char volume_name[256];
  uint64_t num_data_blocks;
  uint64_t num_inodes;
  uint64_t uuid1;
  uint64_t uuid2;
} __attribute__((packed)) MetadataBlock;

typedef struct {
  uint64_t byte_size, block_size;
  uint64_t permissions;
  uint64_t creation_time, modification_time;
  uint64_t flags;
  uint32_t link_count;
  uint32_t checksum;
  uint64_t direct_blocks[38];
  uint64_t indirect_blocks[10];
  uint64_t double_indirect_blocks[5];
  uint64_t triple_indirect_blocks[4];
} __attribute__((packed)) Inode;

struct _Directory {
  uint64_t inode_number;
  uint64_t seek_position;
};

struct _File {
  uint64_t inode_number;
  uint64_t seek_position;
};

static inline void print_block(const uint8_t *const block) {
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 32; ++j) {
      text_output_printf("%02x ", block[i * 32 + j]);
    }
    text_output_printf("\n");
  }
}

static inline uint64_t bytes_to_blocks(uint64_t bytes) {
  return ((bytes - 1) / FILESYSTEM_BLOCK_SIZE_BYTES) + 1;
}

static inline uint64_t bitmap_block_size(uint64_t num_entries) {
  return ((num_entries - 1) / BITMAP_ENTRIES_PER_BLOCK) + 1;
}

// Disk layout: Metadata block, block bitmap, inode bitmap, inodes, data blocks
static inline uint64_t num_inodes(Filesystem *filesystem) {
  MFSData *data = (MFSData *)filesystem->data;
  return data->num_inodes;
}

static inline uint64_t num_data_blocks(Filesystem *filesystem) {
  MFSData *data = (MFSData *)filesystem->data;
  return data->num_data_blocks;
}

static inline uint64_t block_bitmap_size_blocks(Filesystem *filesystem) {
  return bitmap_block_size(num_data_blocks(filesystem));
}

static inline uint64_t inode_bitmap_size_blocks(Filesystem *filesystem) {
  return bitmap_block_size(num_inodes(filesystem));
}
static inline uint64_t block_bitmap_start() { return 1; }

static inline uint64_t inode_bitmap_start(Filesystem *filesystem) {
  return block_bitmap_start() + block_bitmap_size_blocks(filesystem);
}

static inline uint64_t inode_block_start(Filesystem *filesystem) {
  return inode_bitmap_start(filesystem) + inode_bitmap_size_blocks(filesystem);
}

static inline uint64_t data_block_start(Filesystem *filesystem) {
  MFSData *data = (MFSData *)filesystem->data;
  return inode_block_start(filesystem) + data->num_inodes;
}

static inline uint64_t root_directory_inode_number() { return 0; }

WARN_UNUSED
static FilesystemError mark_bitmap_no_read(Filesystem *filesystem,
                                           uint64_t *bitmap,
                                           uint64_t block_number,
                                           int block_bit_index, bool in_use) {
  const int word_index = block_bit_index / BITMAP_ENTRIES_PER_WORD;
  const int word_bit_index = block_bit_index % BITMAP_ENTRIES_PER_WORD;
  if (in_use) {
    bitmap[word_index] |= (1 << word_bit_index);
  } else {
    bitmap[word_index] &= ~(1 << word_bit_index);
  }

  const FilesystemError error =
      filesystem->write_blocks(filesystem, block_number, 1, bitmap);
  if (error != FS_ERROR_NONE) return error;

  return FS_ERROR_NONE;
}

// static FilesystemError mark_bitmap(Filesystem *filesystem, uint64_t
// bitmap_start,
//                                    uint64_t bit_number, bool in_use) {
//   const uint64_t block_number = bitmap_start + bit_number /
//   BITMAP_ENTRIES_PER_BLOCK;
//   FilesystemBlock block;
//   const FilesystemError error = filesystem->read_blocks(filesystem,
//   block_number, 1, &block);
//   if (error != FS_ERROR_NONE) return error;

//   return mark_bitmap_no_read(filesystem, &block, block_number, bit_number %
//   BITMAP_ENTRIES_PER_BLOCK, in_use);
// }

WARN_UNUSED
static FilesystemError bitmap_set_next_free_bit(Filesystem *filesystem,
                                                uint64_t bitmap_start,
                                                uint64_t bitmap_num_bits,
                                                uint64_t *bit_number) {
  uint64_t bitmap[FILESYSTEM_BLOCK_SIZE_BYTES / sizeof(uint64_t)];
  // Iterate over all of the blocks in the bitmap, checking each for a zero bit
  const uint64_t bitmap_num_blocks =
      ((bitmap_num_bits - 1) / BITMAP_ENTRIES_PER_BLOCK) + 1;
  uint64_t bits_left = bitmap_num_bits;
  for (uint64_t block_index = 0; block_index < bitmap_num_blocks;
       ++block_index) {
    const uint64_t block_number = bitmap_start + block_index;
    FilesystemError error =
        filesystem->read_blocks(filesystem, block_number, 1, &bitmap);
    if (error != FS_ERROR_NONE) return error;

    // Check every bit in this block that is part of the bitmap to see if it is
    // free
    const uint64_t num_bits = bits_left > BITMAP_ENTRIES_PER_BLOCK
                                  ? BITMAP_ENTRIES_PER_BLOCK
                                  : bits_left;
    const uint64_t num_words = ((num_bits - 1) / BITMAP_ENTRIES_PER_WORD) + 1;
    for (uint64_t word_index = 0; word_index < num_words; ++word_index) {
      if (bitmap[word_index] != UINT64_MAX) {
        // Find the first set bit (one-indexed) in the inverted word
        const int zero_index = __builtin_ffsll(~bitmap[word_index]) - 1;
        // Skip free bits past the end of the bitmap
        if (word_index * BITMAP_ENTRIES_PER_WORD + zero_index < bits_left) {
          const uint64_t block_bit_index =
              word_index * BITMAP_ENTRIES_PER_WORD + zero_index;
          *bit_number =
              block_index * BITMAP_ENTRIES_PER_BLOCK + block_bit_index;
          return mark_bitmap_no_read(filesystem, bitmap, block_number,
                                     block_bit_index, true);
        }
      }
    }

    bits_left -= BITMAP_ENTRIES_PER_BLOCK;
  }

  return FS_ERROR_OUT_OF_SPACE;
}

WARN_UNUSED
static FilesystemError write_inode(Filesystem *filesystem,
                                   uint64_t inode_number, const Inode *inode) {
  return filesystem->write_blocks(
      filesystem, inode_block_start(filesystem) + inode_number, 1, inode);
}

WARN_UNUSED
static FilesystemError read_inode(Filesystem *filesystem, uint64_t inode_number,
                                  Inode *inode) {
  return filesystem->read_blocks(
      filesystem, inode_block_start(filesystem) + inode_number, 1, inode);
}

WARN_UNUSED
static uint32_t inode_compute_checksum(const Inode *inode) {
  uint32_t checksum = 0;
  uint32_t *words = UNION_CAST(inode, uint32_t *);
  for (size_t i = 0; i < sizeof(Inode) / sizeof(uint32_t); ++i) {
    checksum += words[i];
  }
  checksum -= inode->checksum;

  return checksum;
}

static bool inode_check_checksum(const Inode *inode) {
  return inode->checksum == inode_compute_checksum(inode);
}

// Returns the number of block numbers that can be stored with the specified
// level of indirection (e.g., level zero is one direct block, level 2 is one
// doubly-indirect block).
static inline uint64_t block_numbers_per_indirect_block(int level) {
  uint64_t num_block_numbers = 1;
  for (int i = 0; i < level; ++i) {
    num_block_numbers *= BLOCK_NUMBERS_PER_INDIRECT_BLOCK;
  }
  return num_block_numbers;
}

// Note: indices must have space for 4 uint64_t's
// TODO: Remove Inode* parameter
WARN_UNUSED
static FilesystemError inode_block_indices(const Inode *const inode,
                                           uint64_t block_index,
                                           int *num_indices,
                                           uint64_t *indices) {
  // Number of blocks in each level of (in)direct blocks
  const uint64_t num_blocks_per_level[4] = {
      ITEM_COUNT(inode->direct_blocks), ITEM_COUNT(inode->indirect_blocks),
      ITEM_COUNT(inode->double_indirect_blocks),
      ITEM_COUNT(inode->triple_indirect_blocks)};

  // Block index limit for the previous level of blocks
  uint64_t previous_level_limit = 0;
  for (int i = 0; i < 4; ++i) {
    const uint64_t level_limit =
        previous_level_limit +
        num_blocks_per_level[i] * block_numbers_per_indirect_block(i);

    if (block_index < level_limit) {
      *num_indices = i + 1;
      uint64_t relative_block_index = block_index - previous_level_limit;
      for (int j = i; j >= 0; --j) {
        indices[j] = relative_block_index % BLOCK_NUMBERS_PER_INDIRECT_BLOCK;
        relative_block_index /= BLOCK_NUMBERS_PER_INDIRECT_BLOCK;
      }
      return FS_ERROR_NONE;
    }
    previous_level_limit = level_limit;
  }
  return FS_ERROR_INVALID_PARAMETERS;
}

WARN_UNUSED
static FilesystemError block_number_from_indirect_block(
    Filesystem *filesystem, const Inode *const inode, int num_indices,
    uint64_t *indices, uint64_t *block_number) {
  assert(num_indices > 0 && num_indices < 4);
  // Index 0 is the index into the (in)direct block array in the inode
  const uint64_t *block_level_array[4] = {
      inode->direct_blocks, inode->indirect_blocks,
      inode->double_indirect_blocks, inode->triple_indirect_blocks};
  uint64_t next_block_number = block_level_array[num_indices - 1][indices[0]];

  uint64_t indirect_block[FILESYSTEM_BLOCK_SIZE_BYTES / sizeof(uint64_t)];
  for (int i = 1; i < num_indices; ++i) {
    const FilesystemError error = filesystem->read_blocks(
        filesystem, next_block_number, 1, &indirect_block);
    if (error != FS_ERROR_NONE) return error;
    next_block_number = indirect_block[indices[i]];
  }
  *block_number = next_block_number;

  return FS_ERROR_NONE;
}

WARN_UNUSED
static FilesystemError inode_block_index_to_block_number(
    Filesystem *filesystem, const Inode *const inode, uint64_t block_index,
    uint64_t *block_number) {
  int num_indices;
  uint64_t indices[4] = {0};
  FilesystemError error =
      inode_block_indices(inode, block_index, &num_indices, indices);
  if (error != FS_ERROR_NONE) return error;

  return block_number_from_indirect_block(filesystem, inode, num_indices,
                                          indices, block_number);
}

// Note: Does *not* write the inode to the filesystem, calling this function
// *must* be followed by a call to write_inode().
WARN_UNUSED
static FilesystemError inode_set_block_number(Filesystem *filesystem,
                                              Inode *inode, int num_indices,
                                              uint64_t *indices,
                                              uint64_t data_block_number) {
  assert(num_indices > 0 && num_indices < 4);

  // Count how many ending indices are 0 (which means they are the first in a
  // new block). We exclude the first index, since those are a part of the
  // inode, and therefore always allocated).
  int num_unallocated = 0;
  for (int i = num_indices - 1; i > 0; --i) {
    if (indices[i] != 0) break;
    ++num_unallocated;
  }

  uint64_t *const block_level_array[4] = {
      inode->direct_blocks, inode->indirect_blocks,
      inode->double_indirect_blocks, inode->triple_indirect_blocks};

  // If the chain is partially allocated, determine the block number of the last
  // allocated block.
  FilesystemError error = FS_ERROR_NONE;
  uint64_t previous_indirect_block_number = -1;
  uint64_t indirect_block[FILESYSTEM_BLOCK_SIZE_BYTES / sizeof(uint64_t)];
  if (num_unallocated < num_indices - 1) {
    error = block_number_from_indirect_block(
        filesystem, inode, num_indices - num_unallocated - 1, indices,
        &previous_indirect_block_number);
    if (error != FS_ERROR_NONE) return error;

    error = filesystem->read_blocks(filesystem, previous_indirect_block_number,
                                    1, &indirect_block);
    if (error != FS_ERROR_NONE) return error;
  }

  for (int i = num_indices - num_unallocated - 1; i < num_indices; ++i) {
    // Determine the block number to set at this level
    uint64_t block_number;
    if (i == num_indices - 1) {
      block_number = data_block_number;
    } else {
      error =
          bitmap_set_next_free_bit(filesystem, block_bitmap_start(),
                                   num_data_blocks(filesystem), &block_number);
      if (error != FS_ERROR_NONE) return error;
    }

    if (i == 0) {  // Set the block in the inode
      block_level_array[num_indices - 1][indices[0]] = block_number;
    } else {  // Set an indirect block
      indirect_block[indices[i]] = block_number;
      error = filesystem->write_blocks(
          filesystem, previous_indirect_block_number, 1, &indirect_block);
      if (error != FS_ERROR_NONE) return error;
      memset(&indirect_block, 0, sizeof(indirect_block));

      previous_indirect_block_number = block_number;
    }
  }

  return FS_ERROR_NONE;
}

WARN_UNUSED
static FilesystemError inode_add_block(Filesystem *filesystem, Inode *inode,
                                       uint64_t inode_number,
                                       uint64_t *block_number) {
  const uint64_t next_block_index =
      inode->byte_size / FILESYSTEM_BLOCK_SIZE_BYTES;
  int num_indices;
  uint64_t indices[4] = {0};
  FilesystemError error =
      inode_block_indices(inode, next_block_index, &num_indices, indices);
  if (error != FS_ERROR_NONE) return error;

  // Allocate our new data block for the inode
  uint64_t data_block_index;
  error =
      bitmap_set_next_free_bit(filesystem, block_bitmap_start(),
                               num_data_blocks(filesystem), &data_block_index);
  *block_number = data_block_start(filesystem) + data_block_index;
  if (error != FS_ERROR_NONE) return error;
  error = inode_set_block_number(filesystem, inode, num_indices, indices,
                                 *block_number);
  if (error != FS_ERROR_NONE) return error;

  inode->block_size++;
  return write_inode(filesystem, inode_number, inode);
}

// TODO: Test this
WARN_UNUSED
static FilesystemError path_to_inode_number(Filesystem *filesystem,
                                            const char *path,
                                            uint64_t *inode_number) {
  // Only absolute paths are allowed
  if (path[0] != '/') {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  *inode_number = root_directory_inode_number();
  const char *directory_name_start = path + 1;
  while (true) {
    // Determine directory name
    const char *next_slash = strchrnul(directory_name_start, '/');
    const size_t name_length = next_slash - directory_name_start;
    if (name_length > FILESYSTEM_MAX_DIR_NAME_LENGTH)
      return FS_ERROR_INVALID_PARAMETERS;
    if (name_length == 0) return FS_ERROR_NONE;  // Trailing slash

    // Read current directory to find next directory
    Inode inode;
    FilesystemError error = read_inode(filesystem, *inode_number, &inode);
    if (error != FS_ERROR_NONE) return error;
    assert(inode.byte_size % sizeof(DirectoryEntry) == 0);

    // Read each data block in the inode and linearly search for the directory
    // name
    bool found = false;
    uint64_t directory_entries_left = inode.byte_size / sizeof(DirectoryEntry);
    for (uint64_t block_index = 0; block_index < inode.block_size;
         ++block_index) {
      uint64_t block_number = 0;
      FilesystemError error = inode_block_index_to_block_number(
          filesystem, &inode, block_index, &block_number);
      if (error != FS_ERROR_NONE) return error;

      FilesystemBlock directory_data_block;
      error = filesystem->read_blocks(filesystem, block_number, 1,
                                      &directory_data_block);
      if (error != FS_ERROR_NONE) return error;

      const DirectoryEntry *directory_entries =
          (DirectoryEntry *)directory_data_block.data;
      const uint64_t num_entries =
          directory_entries_left > DIRECTORY_ENTRIES_PER_BLOCK
              ? DIRECTORY_ENTRIES_PER_BLOCK
              : directory_entries_left;
      for (uint64_t entry_index = 0; entry_index < num_entries; ++entry_index) {
        if (strncmp(directory_entries[entry_index].name, directory_name_start,
                    name_length) == 0) {
          // TODO: DirectoryEntry shouldn't be public?
          *inode_number = directory_entries[entry_index].id;
          found = true;
          break;
        }
      }

      if (found) break;
    }

    if (!found) break;

    // If we've reached the end of the path, return
    if (*next_slash == '\0') return FS_ERROR_NONE;
    directory_name_start = next_slash + 1;
  }

  // The path could not be found
  return FS_ERROR_NOT_FOUND;
}

static FilesystemError read_data_from_inode(Filesystem *filesystem,
                                            const Inode *const inode,
                                            const uint64_t start_offset,
                                            uint8_t *const buffer,
                                            uint64_t *const length) {
  if (start_offset >= inode->byte_size) {
    *length = 0;
    return FS_ERROR_NONE;
  }

  // Don't read past the end of the data.
  *length = min(*length, inode->byte_size - start_offset);

  uint64_t bytes_read = 0;
  while (bytes_read < *length) {
    const uint64_t block_index =
        (start_offset + bytes_read) / FILESYSTEM_BLOCK_SIZE_BYTES;
    const uint64_t block_offset =
        (start_offset + bytes_read) % FILESYSTEM_BLOCK_SIZE_BYTES;

    uint64_t block_number = -1;
    FilesystemError error = inode_block_index_to_block_number(
        filesystem, inode, block_index, &block_number);
    if (error != FS_ERROR_NONE) return error;

    // Read a whole block, if possible.
    // TODO: Read multiple blocks if they're consecutive.
    if (block_offset == 0 &&
        (*length - bytes_read) > FILESYSTEM_BLOCK_SIZE_BYTES) {
      error = filesystem->read_blocks(filesystem, block_number, 1,
                                      &buffer[bytes_read]);
      if (error != FS_ERROR_NONE) return error;
      bytes_read += FILESYSTEM_BLOCK_SIZE_BYTES;
    } else {
      FilesystemBlock data_block;
      error = filesystem->read_blocks(filesystem, block_number, 1, &data_block);
      if (error != FS_ERROR_NONE) return error;

      const uint64_t bytes_to_read =
          min(*length - bytes_read, FILESYSTEM_BLOCK_SIZE_BYTES - block_offset);
      memcpy(&buffer[bytes_read], &data_block.data[block_offset],
             bytes_to_read);
      bytes_read += bytes_to_read;
    }
  }

  *length = bytes_read;
  return FS_ERROR_NONE;
}

static FilesystemError write_data_to_inode(Filesystem *filesystem, Inode *inode,
                                           uint64_t inode_number,
                                           const uint64_t start_offset,
                                           const uint8_t *buffer,
                                           const uint64_t length) {
  // Determine where to add the new data, addings blocks to the inode if
  // necessary.
  uint64_t bytes_written = 0;
  FilesystemError error = FS_ERROR_NONE;
  while (bytes_written < length) {
    uint64_t current_block_index =
        (start_offset + bytes_written) / FILESYSTEM_BLOCK_SIZE_BYTES;
    uint64_t current_block_offset =
        (start_offset + bytes_written) % FILESYSTEM_BLOCK_SIZE_BYTES;

    // TODO: Add multiple blocks at once
    while (current_block_index >= inode->block_size) {  // Allocate a new block
      uint64_t block_number = -1;
      error = inode_add_block(filesystem, inode, inode_number, &block_number);
      if (error != FS_ERROR_NONE) return error;
    }

    uint64_t current_block_number;
    error = inode_block_index_to_block_number(
        filesystem, inode, current_block_index, &current_block_number);
    if (error != FS_ERROR_NONE) return error;

    // Determine if we can write a full block at once.
    // TODO: Write more than one block at once.
    if (current_block_offset == 0 &&
        (length - bytes_written) >= FILESYSTEM_BLOCK_SIZE_BYTES) {
      error = filesystem->write_blocks(filesystem, current_block_number, 1,
                                       &buffer[bytes_written]);
      if (error != FS_ERROR_NONE) return error;
      bytes_written += FILESYSTEM_BLOCK_SIZE_BYTES;
    } else {  // Write a partial block
      FilesystemBlock block = {0};
      if (start_offset + bytes_written < inode->byte_size ||
          start_offset != 0) {
        // This is being written into an existing block, read it first.
        error = filesystem->read_blocks(filesystem, current_block_number, 1,
                                        &block);
        if (error != FS_ERROR_NONE) return error;
      }

      const uint64_t bytes_to_write =
          min(length - bytes_written,
              FILESYSTEM_BLOCK_SIZE_BYTES - current_block_offset);
      memcpy(&block.data[current_block_offset], buffer, bytes_to_write);
      error =
          filesystem->write_blocks(filesystem, current_block_number, 1, &block);
      if (error != FS_ERROR_NONE) return error;
      bytes_written += bytes_to_write;
    }

    inode->byte_size = max(inode->byte_size, start_offset + bytes_written);
  }

  inode->checksum = inode_compute_checksum(inode);
  return write_inode(filesystem, inode_number, inode);
}

static FilesystemError create_directory_entry(
    Filesystem *filesystem, const char *name,
    const Directory *const parent_directory, const uint64_t flags,
    uint64_t *new_inode_number) {
  // Don't allow empty names.
  if (name[0] == '\0') {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  MFSData *data = (MFSData *)filesystem->data;
  FilesystemError error =
      bitmap_set_next_free_bit(filesystem, inode_bitmap_start(filesystem),
                               data->num_inodes, new_inode_number);
  if (error != FS_ERROR_NONE) return error;

  // Read the parent directory's inode and check it's validity.
  Inode parent_inode;
  error = read_inode(filesystem, parent_directory->inode_number, &parent_inode);
  if (error != FS_ERROR_NONE) return error;

  if (!inode_check_checksum(&parent_inode) ||
      (parent_inode.byte_size % sizeof(DirectoryEntry)) != 0) {
    return FS_ERROR_CORRUPT_FILESYSTEM;
  }
  if ((parent_inode.flags & INODE_FLAG_DIRECTORY) == 0) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  DirectoryEntry new_entry = {0};
  new_entry.id = *new_inode_number;
  strlcpy(new_entry.name, name, sizeof(new_entry.name));
  error = write_data_to_inode(
      filesystem, &parent_inode, parent_directory->inode_number,
      parent_inode.byte_size, UNION_CAST(&new_entry, const uint8_t *),
      sizeof(new_entry));
  if (error != FS_ERROR_NONE) return error;

  // Create and write the new inode
  Inode new_inode;
  memset(&new_inode, 0, sizeof(new_inode));
  new_inode.flags = flags;
  new_inode.link_count = 1;
  new_inode.checksum = inode_compute_checksum(&new_inode);

  error = write_inode(filesystem, *new_inode_number, &new_inode);
  if (error != FS_ERROR_NONE) return error;
  return FS_ERROR_NONE;
}

static FilesystemError open_inode(Filesystem *filesystem,
                                  uint64_t inode_number) {
  MFSData *const data = (MFSData *)filesystem->data;
  ListEntry *const new_entry = kmalloc(sizeof(ListEntry));
  if (!new_entry) return FS_ERROR_DEVICE_ERROR;
  list_entry_set_value(new_entry, inode_number);
  list_push_front(&data->open_inodes, new_entry);

  return FS_ERROR_NONE;
}

static FilesystemError close_inode(Filesystem *filesystem,
                                   uint64_t inode_number) {
  MFSData *const data = (MFSData *)filesystem->data;
  ListEntry *entry = list_head(&data->open_inodes);
  while (entry) {
    if (list_entry_value(entry) == inode_number) {
      list_remove(&data->open_inodes, entry);
      return FS_ERROR_NONE;
    }
    entry = list_next(entry);
  }

  return FS_ERROR_NOT_FOUND;
}

static FilesystemError open_directory(Filesystem *filesystem,
                                      uint64_t inode_number,
                                      Directory **directory) {
  const FilesystemError error = open_inode(filesystem, inode_number);
  if (error != FS_ERROR_NONE) return error;

  *directory = kmalloc(sizeof(Directory));  // Freed on close
  if (!directory) return FS_ERROR_DEVICE_ERROR;
  (*directory)->inode_number = inode_number;
  (*directory)->seek_position = 0;

  return FS_ERROR_NONE;
}

static FilesystemError open_file(Filesystem *filesystem, uint64_t inode_number,
                                 File **file) {
  const FilesystemError error = open_inode(filesystem, inode_number);
  if (error != FS_ERROR_NONE) return error;

  *file = kmalloc(sizeof(File));  // Freed on close
  if (!file) return FS_ERROR_DEVICE_ERROR;
  (*file)->inode_number = inode_number;
  (*file)->seek_position = 0;

  return FS_ERROR_NONE;
}

static void store_metadata(Filesystem *filesystem,
                           const MetadataBlock *metadata) {
  MFSData *data = (MFSData *)filesystem->data;
  data->size_blocks = metadata->size_blocks;
  strlcpy(data->volume_name, metadata->volume_name, sizeof(data->volume_name));
  data->num_data_blocks = metadata->num_data_blocks;
  data->num_inodes = metadata->num_inodes;
  data->uuid1 = metadata->uuid1;
  data->uuid2 = metadata->uuid2;
}

WARN_UNUSED
static FilesystemError sata_read_blocks(Filesystem *filesystem,
                                        uint64_t block_start,
                                        uint64_t num_blocks, void *blocks) {
  MFSData *data = (MFSData *)filesystem->data;
  const uint32_t sector_size = data->device.sata.sector_size;
  assert(FILESYSTEM_BLOCK_SIZE_BYTES % sector_size == 0);
  if (data->device.sata.driver == NULL) {
    return FS_ERROR_DEVICE_ERROR;
  }

  if (block_start + num_blocks > data->size_blocks) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  struct AHCIReadCommand read_command = {
      .device_id = data->device.sata.device_id,
      .address = block_start,
      .block_count = num_blocks * FILESYSTEM_BLOCK_SIZE_BYTES / sector_size};
  const PCIDeviceDriverError error = data->device.sata.driver->execute_command(
      data->device.sata.driver, AHCI_COMMAND_READ, &read_command,
      sizeof(read_command), blocks, num_blocks * FILESYSTEM_BLOCK_SIZE_BYTES);

  return error == PCI_ERROR_NONE ? FS_ERROR_NONE : FS_ERROR_DEVICE_ERROR;
}

WARN_UNUSED
static FilesystemError sata_write_blocks(Filesystem *filesystem,
                                         uint64_t block_start,
                                         uint64_t num_blocks,
                                         const void *blocks) {
  MFSData *data = (MFSData *)filesystem->data;
  const uint32_t sector_size = data->device.sata.sector_size;
  assert(FILESYSTEM_BLOCK_SIZE_BYTES % sector_size == 0);
  if (data->device.sata.driver == NULL) {
    return FS_ERROR_DEVICE_ERROR;
  }

  if (block_start + num_blocks > data->size_blocks) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  struct AHCIWriteCommand write_command = {
      .device_id = data->device.sata.device_id,
      .address = block_start,
      .block_count = num_blocks * FILESYSTEM_BLOCK_SIZE_BYTES / sector_size};
  const PCIDeviceDriverError error = data->device.sata.driver->execute_command(
      data->device.sata.driver, AHCI_COMMAND_WRITE, &write_command,
      sizeof(write_command), (void *)blocks,
      num_blocks * FILESYSTEM_BLOCK_SIZE_BYTES);

  return error == PCI_ERROR_NONE ? FS_ERROR_NONE : FS_ERROR_DEVICE_ERROR;
}

WARN_UNUSED
static FilesystemError in_memory_read_blocks(Filesystem *filesystem,
                                             uint64_t block_start,
                                             uint64_t num_blocks,
                                             void *blocks) {
  MFSData *data = (MFSData *)filesystem->data;
  if (data->device.in_memory.blocks == NULL) {
    return FS_ERROR_DEVICE_ERROR;
  }

  if (block_start + num_blocks > data->size_blocks) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  // TODO: Remove copy for in-memory reads
  memcpy(blocks, &data->device.in_memory.blocks[block_start],
         num_blocks * FILESYSTEM_BLOCK_SIZE_BYTES);
  return FS_ERROR_NONE;
}

WARN_UNUSED
static FilesystemError in_memory_write_blocks(Filesystem *filesystem,
                                              uint64_t block_start,
                                              uint64_t num_blocks,
                                              const void *blocks) {
  MFSData *data = (MFSData *)filesystem->data;
  if (data->device.in_memory.blocks == NULL) {
    return FS_ERROR_DEVICE_ERROR;
  }

  if (block_start + num_blocks > data->size_blocks) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  memcpy(&data->device.in_memory.blocks[block_start], blocks,
         num_blocks * FILESYSTEM_BLOCK_SIZE_BYTES);
  return FS_ERROR_NONE;
}

WARN_UNUSED
static FilesystemError mfs_init(Filesystem *filesystem) {
  if (filesystem == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  assert(sizeof(MetadataBlock) < FILESYSTEM_BLOCK_SIZE_BYTES);
  assert(sizeof(Inode) == FILESYSTEM_BLOCK_SIZE_BYTES);
  assert(FILESYSTEM_BLOCK_SIZE_BYTES % sizeof(DirectoryEntry) == 0);

  MFSData *data = (MFSData *)filesystem->data;
  list_init(&data->open_inodes);

  FilesystemBlock metadata_block;
  FilesystemError error =
      filesystem->read_blocks(filesystem, 0, 1, &metadata_block);
  if (error != FS_ERROR_NONE) return error;

  MetadataBlock *metadata = UNION_CAST(&metadata_block, MetadataBlock *);
  if (strncmp(metadata->magic, MFS_MAGIC_STRING, sizeof(MFS_MAGIC_STRING)) ==
      0) {
    data->formatted = true;
    store_metadata(filesystem, metadata);
  } else {
    data->formatted = false;
  }

  return FS_ERROR_NONE;
}

static FilesystemError mfs_init_in_memory(
    Filesystem *filesystem, const void *const initialization_data) {
  if (filesystem == NULL || initialization_data == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  const MFSInMemoryInitData *const init =
      (MFSInMemoryInitData *)initialization_data;
  if (init->blocks == NULL) return FS_ERROR_INVALID_PARAMETERS;

  // TODO: Free this somewhere
  MFSData *data = kcalloc(1, sizeof(MFSData));
  data->device.in_memory.blocks = init->blocks;
  data->size_blocks = init->num_blocks;

  filesystem->data = data;
  return mfs_init(filesystem);
}

static FilesystemError mfs_init_sata(Filesystem *filesystem,
                                     const void *const initialization_data) {
  if (filesystem == NULL || initialization_data == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  const MFSSATAInitData *const init = (MFSSATAInitData *)initialization_data;
  // TODO: Free this somewhere
  MFSData *data = kcalloc(1, sizeof(MFSData));
  data->device.sata.driver = init->driver;
  data->device.sata.device_id = init->device_id;

  const struct AHCIDeviceInfoCommand info_request = {.device_id =
                                                         init->device_id};
  struct AHCIDeviceInfo info;
  const PCIDeviceDriverError ahci_error = init->driver->execute_command(
      init->driver, AHCI_COMMAND_DEVICE_INFO, &info_request,
      sizeof(info_request), &info, sizeof(info));
  if (ahci_error != PCI_ERROR_NONE) return FS_ERROR_DEVICE_ERROR;
  if (info.device_type != AHCI_DEVICE_SATA) return FS_ERROR_INVALID_PARAMETERS;
  if (FILESYSTEM_BLOCK_SIZE_BYTES % info.logical_sector_size != 0) {
    return FS_ERROR_INVALID_PARAMETERS;
  }
  data->device.sata.sector_size = info.logical_sector_size;
  data->size_blocks =
      info.num_sectors * info.logical_sector_size / FILESYSTEM_BLOCK_SIZE_BYTES;

  filesystem->data = data;
  return mfs_init(filesystem);
}

static FilesystemError mfs_format(Filesystem *filesystem,
                                  const char const *volume_name,
                                  uint64_t size_blocks) {
  if (filesystem == NULL || volume_name == NULL || size_blocks == 0) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  MFSData *data = (MFSData *)filesystem->data;
  data->formatted = true;

  // Populate and write the first, metadata block
  FilesystemBlock metadata_block;
  MetadataBlock *metadata = UNION_CAST(&metadata_block, MetadataBlock *);
  memcpy(metadata->magic, MFS_MAGIC_STRING, sizeof(metadata->magic));
  metadata->size_blocks = size_blocks;
  // TODO: Determine how to figure this out
  metadata->num_inodes = size_blocks / 10;

  const uint64_t inode_bitmap_size_blocks =
      bitmap_block_size(metadata->num_inodes);
  const uint64_t block_bitmap_size_blocks_estimated = bitmap_block_size(
      size_blocks - metadata->num_inodes - inode_bitmap_size_blocks - 1);
  metadata->num_data_blocks = size_blocks - 1 -
                              block_bitmap_size_blocks_estimated -
                              inode_bitmap_size_blocks - metadata->num_inodes;
  // TODO: Fill UUID
  strlcpy(metadata->volume_name, volume_name, sizeof(metadata->volume_name));
  filesystem->write_blocks(filesystem, 0, 1, &metadata_block);
  store_metadata(filesystem, metadata);

  // Zero the free-block bitmap
  // TODO: Zero multiple blocks at once
  FilesystemBlock zero_block;
  memset(&zero_block, 0, sizeof(zero_block));
  for (uint64_t i = 0; i < block_bitmap_size_blocks(filesystem); ++i) {
    const FilesystemError error = filesystem->write_blocks(
        filesystem, block_bitmap_start() + i, 1, &zero_block);
    if (error != FS_ERROR_NONE) return error;
  }

  // Zero the free-inode bitmap
  for (uint64_t i = 0; i < inode_bitmap_size_blocks; ++i) {
    const FilesystemError error = filesystem->write_blocks(
        filesystem, inode_bitmap_start(filesystem) + i, 1, &zero_block);
    if (error != FS_ERROR_NONE) return error;
  }

  // Create the root-directory
  // Mark the root-directory inode as non-free
  uint64_t inode_number = -1;
  FilesystemError error =
      bitmap_set_next_free_bit(filesystem, inode_bitmap_start(filesystem),
                               data->num_inodes, &inode_number);
  if (error != FS_ERROR_NONE) return error;
  assert(inode_number == root_directory_inode_number());

  // Zero the root-directory inode
  Inode root_directory_inode;
  memset(&root_directory_inode, 0, sizeof(root_directory_inode));
  root_directory_inode.flags = INODE_FLAG_DIRECTORY;
  root_directory_inode.link_count = 1;
  root_directory_inode.checksum = inode_compute_checksum(&root_directory_inode);
  error = write_inode(filesystem, root_directory_inode_number(),
                      &root_directory_inode);
  if (error != FS_ERROR_NONE) return error;

  return FS_ERROR_NONE;
}

static FilesystemError mfs_info(Filesystem *filesystem, FilesystemInfo *info) {
  if (filesystem == NULL || info == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  MFSData *data = (MFSData *)filesystem->data;
  info->formatted = data->formatted;
  info->size_blocks = data->size_blocks;
  strlcpy(info->volume_name, data->volume_name, sizeof(info->volume_name));

  return FS_ERROR_NONE;
}

static FilesystemError mfs_create_directory(
    Filesystem *filesystem, const char *name,
    const Directory *const parent_directory, Directory **directory) {
  if (filesystem == NULL || name == NULL || parent_directory == NULL ||
      directory == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  uint64_t new_inode_number = -1;
  const FilesystemError error =
      create_directory_entry(filesystem, name, parent_directory,
                             INODE_FLAG_DIRECTORY, &new_inode_number);
  if (error != FS_ERROR_NONE) return error;

  return open_directory(filesystem, new_inode_number, directory);
}

static FilesystemError mfs_open_directory(Filesystem *filesystem,
                                          const char const *path,
                                          Directory **directory) {
  if (filesystem == NULL || path == NULL || directory == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  uint64_t inode_number;
  FilesystemError error = path_to_inode_number(filesystem, path, &inode_number);
  if (error != FS_ERROR_NONE) return error;

  return open_directory(filesystem, inode_number, directory);
}

static FilesystemError mfs_close_directory(Filesystem *filesystem,
                                           Directory *directory) {
  if (filesystem == NULL || directory == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  const FilesystemError error =
      close_inode(filesystem, directory->inode_number);
  if (error == FS_ERROR_NONE) {
    kfree(directory);
  }

  return error;
}

static FilesystemError mfs_read_directory_entry(Filesystem *filesystem,
                                                Directory *const directory,
                                                DirectoryEntry *const entry) {
  if (filesystem == NULL || directory == NULL || entry == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  Inode inode;
  FilesystemError error =
      read_inode(filesystem, directory->inode_number, &inode);
  if (error != FS_ERROR_NONE) return error;

  if (!inode_check_checksum(&inode) ||
      inode.byte_size % sizeof(DirectoryEntry) != 0) {
    return FS_ERROR_CORRUPT_FILESYSTEM;
  }
  if ((inode.flags & INODE_FLAG_DIRECTORY) == 0) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  uint64_t length = sizeof(DirectoryEntry);
  error = read_data_from_inode(
      filesystem, &inode, directory->seek_position * sizeof(DirectoryEntry),
      UNION_CAST(entry, uint8_t *), &length);

  if (length != sizeof(DirectoryEntry)) {
    entry->id = -1;
    entry->name[0] = '\0';
  } else {
    directory->seek_position++;
  }

  return FS_ERROR_NONE;
}

static FilesystemError mfs_seek_front_directory(Filesystem *filesystem,
                                                Directory *const directory) {
  if (filesystem == NULL || directory == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  directory->seek_position = 0;
  return FS_ERROR_NONE;
}

static FilesystemError mfs_create_file(Filesystem *filesystem,
                                       const Directory *parent_directory,
                                       const char const *name, File **file) {
  if (filesystem == NULL || name == NULL || parent_directory == NULL ||
      file == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  uint64_t new_inode_number = -1;
  const FilesystemError error = create_directory_entry(
      filesystem, name, parent_directory, INODE_FLAG_NONE, &new_inode_number);
  if (error != FS_ERROR_NONE) return error;

  return open_file(filesystem, new_inode_number, file);
}

static FilesystemError mfs_open_file(Filesystem *filesystem,
                                     const char const *path, File **file) {
  if (filesystem == NULL || path == NULL || file == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  uint64_t inode_number;
  FilesystemError error = path_to_inode_number(filesystem, path, &inode_number);
  if (error != FS_ERROR_NONE) return error;

  return open_file(filesystem, inode_number, file);
}

static FilesystemError mfs_close_file(Filesystem *filesystem, File *file) {
  if (filesystem == NULL || file == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  const FilesystemError error = close_inode(filesystem, file->inode_number);
  if (error == FS_ERROR_NONE) {
    kfree(file);
  }

  return error;
}

static FilesystemError mfs_read_file(Filesystem *filesystem, File *const file,
                                     uint64_t *const length, uint8_t *buffer) {
  if (filesystem == NULL || file == NULL || length == 0 || buffer == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  Inode inode;
  FilesystemError error = read_inode(filesystem, file->inode_number, &inode);
  if (error != FS_ERROR_NONE) return error;

  if (!inode_check_checksum(&inode)) {
    return FS_ERROR_CORRUPT_FILESYSTEM;
  }
  if ((inode.flags & INODE_FLAG_DIRECTORY) != 0) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  return read_data_from_inode(filesystem, &inode, file->seek_position, buffer,
                              length);
}

static FilesystemError mfs_write_file(Filesystem *filesystem, File *const file,
                                      const uint8_t *buffer,
                                      const uint64_t length) {
  if (filesystem == NULL || file == NULL || length == 0 || buffer == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  Inode inode;
  FilesystemError error = read_inode(filesystem, file->inode_number, &inode);
  if (error != FS_ERROR_NONE) return error;

  if (!inode_check_checksum(&inode)) {
    return FS_ERROR_CORRUPT_FILESYSTEM;
  }
  if ((inode.flags & INODE_FLAG_DIRECTORY) != 0) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  error = write_data_to_inode(filesystem, &inode, file->inode_number,
                              file->seek_position, buffer, length);
  if (error != FS_ERROR_NONE) return error;

  file->seek_position += length;
  return FS_ERROR_NONE;
}

static FilesystemError mfs_seek_file(Filesystem *filesystem, File *const file,
                                     const uint64_t position) {
  if (filesystem == NULL || file == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  file->seek_position = position;
  return FS_ERROR_NONE;
}

static FilesystemError mfs_tell_file(Filesystem *filesystem, const File *file,
                                     uint64_t *position) {
  if (filesystem == NULL || file == NULL || position == NULL) {
    return FS_ERROR_INVALID_PARAMETERS;
  }

  *position = file->seek_position;
  return FS_ERROR_NONE;
}

static Filesystem mfs_create_default() {
  Filesystem fs = {
      .format = mfs_format,
      .info = mfs_info,
      .create_directory = mfs_create_directory,
      .open_directory = mfs_open_directory,
      .close_directory = mfs_close_directory,
      .read_directory_entry = mfs_read_directory_entry,
      .seek_front_directory = mfs_seek_front_directory,
      .create_file = mfs_create_file,
      .open_file = mfs_open_file,
      .close_file = mfs_close_file,
      .read_file = mfs_read_file,
      .write_file = mfs_write_file,
      .seek_file = mfs_seek_file,
      .tell_file = mfs_tell_file,
  };
  return fs;
}

void mfs_in_memory_register() {
  REQUIRE_MODULE("filesystem");

  Filesystem fs = mfs_create_default();
  fs.identifier = "MFS_M";
  fs.long_name = "MosquitOS Filesystem (in-memory)";
  fs.read_blocks = in_memory_read_blocks;
  fs.write_blocks = in_memory_write_blocks;
  fs.init = mfs_init_in_memory;

  filesystem_register(fs);
}

void mfs_sata_register() {
  REQUIRE_MODULE("filesystem");

  Filesystem fs = mfs_create_default();
  fs.identifier = "MFS_S";
  fs.long_name = "MosquitOS Filesystem (SATA)";
  fs.read_blocks = sata_read_blocks;
  fs.write_blocks = sata_write_blocks;
  fs.init = mfs_init_sata;

  filesystem_register(fs);
}
