 # MFS (MosquitOS Filesystem) #

## Goals ##

  - Simple
  - Easy in-memory representation
  - Decently small overhead
  - Can support large files
  - Can support large volumes
  - Partitions?

## Implementation ##

  - First block contains metadata (magic number, volume size, free size, number of inodes, volume name, 128-bit uuid)
  - Starting at block two is the free-block bitmap - one bit per usable block on the HDD, 0 if free, 1 if used
  - Next is free-inode bitmap
  - Pre-allocated, 1-block inodes that contain an 8-byte size, 8-byte creation/modification time, 8-byte permissions, 4-byte link count, 4-byte checksum, 8-byte flags, 39 direct pointers, 10 singly indirect pointers, 5 doubly indirect pointers, 4 triply indirect pointers.
  - First inode is for the root directory
  - Directories are a null-separated list of file names (255 char max) and inode numbers

## API ##

  - Get required buffer size for creating FS
  - Create filesystem (input size, inode percentage, name, pointer to buffer where FS will be stored)
  
  - Open directory by path (relative or absolute)
  - Close directory
  - Open file by path
  - Close  file
  - Read directory entry
  - Read file bytes
  - Seek to beginning of directory
  - Seek to byte in file
  - Get cursor position in file
  - Create directory
  - Create file
  - Delete directory by path
  - Delete link (directory pointer and name)
  - Create link