# LinuxFileSystem
MyFS - Virtual File System Simulator

1. Project Purpose & Overview

MyFS is a custom, lightweight user-space file system simulator written in C++. It simulates the three primary abstraction layers of a standard operating system storage stack:

Virtual File System (VFS Layer): Provides an interactive command-line interface for user interaction (ls, touch, cat, edit, rm, mkdir, rmdir, mv).

File System (FS Layer): Manages metadata (inodes), file allocation, contiguous data shifting, directory entries, and directory operations.

Block Device Layer: Simulates physical storage by mapping memory to a disk image file via Linux memory mapping (mmap).

2. Compilation and Execution Instructions

Using CMake (Required build flow):

mkdir build
cd build
cmake ..
make
./bin/myfs /tmp/myfs_storage.bin


Using Makefile:

make
./bin/myfs /tmp/myfs_storage.bin


Example Usage Session:

$ ./bin/myfs /tmp/test_storage
Did not find myfs instance on blkdev
Creating...
Finished!
Welcome to myfs

To get help, please type 'help' on the prompt below.

myfs$ touch file_a
myfs$ edit file_a
Enter new file content
This is some file content
myfs$ ls
file_a                   26
myfs$ touch file_b
myfs$ ls
file_a                   26
file_b                   0
myfs$ cat file_a
This is some file content
myfs$ exit


Re-running the simulator on the same file preserves the storage state:

$ ./bin/myfs /tmp/test_storage
Welcome to myfs

To get help, please type 'help' on the prompt below.

myfs$ ls
file_a                   26
file_b                   0


3. Implemented Files and Their Roles

blkdev.h / blkdev.cpp: Implements BlockDeviceSimulator using mmap() to bridge RAM operations with the underlying storage file.

myfs.h / myfs.cpp: Implements the MyFs class, managing the myfs_header, inode table (512 entries), contiguous memory management (fragmentMemory), file allocation, and directory operations.

vfs.h / vfs.cpp: Implements user interaction parsing commands and invoking corresponding MyFs functions.

myfs_main.cpp: Entry point instantiating BlockDeviceSimulator, MyFs, and initiating run_vfs.

CMakeLists.txt: Build configuration emitting executable to ./bin/myfs.

4. Memory Layout & File Management Details

The block device storage layout is structured as follows:

+-------------------+------------------------------------+----------------------------------+
| Header (5 Bytes)  | Inode Table (512 x 32 = 16384 B)   | Data Area (Contiguous Storage)   |
+-------------------+------------------------------------+----------------------------------+


Header: First 5 bytes (MYFS magic string + version byte).

Inode Structure (32 bytes per entry):

file_id (uint32_t, 4 bytes): 0 if free, non-zero if occupied.

name (char[20], 20 bytes): Name of file/directory (max 20 characters).

size (uint32_t, 4 bytes): Size of payload in bytes.

start (uint32_t, 4 bytes): Byte offset address in data area.

is_dir (uint8_t, 1 byte): 0 for regular file, 1 for directory.

padding (uint8_t[3], 3 bytes): Struct alignment to 32 bytes.

Data Management:
Data payload is stored contiguously in the Data Area. When a file grows, shrinks, or is deleted, fragmentMemory() shifts subsequent files dynamically to avoid gaps and keep data packed.

5. Part B (Bonus Implementation: Directories & Move)

Part B is fully implemented:

mkdir <path>: Creates a directory entry (is_dir = 1).

rmdir <path>: Removes a directory entry.

mv <src> <dst>: Renames or moves files/directories by updating the inode name field.