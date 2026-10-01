#ifndef __MYFS_H__
#define __MYFS_H__

#include <memory>
#include <vector>
#include <stdint.h>
#include "blkdev.h"

struct INODE {
    uint32_t file_id;   // 4 bytes
    char name[20];      // 20 bytes
    uint32_t size;      // 4 bytes
    uint32_t start;     // 4 bytes
    uint8_t is_dir;     // 1 byte (0 = file, 1 = directory)
    uint8_t padding[3]; // 3 bytes alignment
};

class MyFs {
public:
    MyFs(BlockDeviceSimulator *blkdevsim_);

    /**
     * format method
     * Discards current content in blockdevice and creates fresh MYFS.
     */
    void format();

    /**
     * create_file method
     * Creates a new file or directory in required path.
     */
    void create_file(std::string path_str, bool directory);

    /**
     * get_content method
     * Returns content of file at path_str.
     */
    std::string get_content(std::string path_str);

    /**
     * set_content method
     * Sets content of file at path_str.
     */
    void set_content(std::string path_str, std::string content);

    /**
     * list_dir method
     * Lists files and directories in path_str.
     */
    void list_dir(std::string path_str);

    /**
     * remove_file method
     * Deletes file at path_str.
     */
    void remove_file(std::string path_str);

    /**
     * remove_dir method
     * Deletes directory at path_str (Bonus Part B).
     */
    void remove_dir(std::string path_str);

    /**
     * move_item method
     * Moves or renames file or directory from src to dst (Bonus Part B).
     */
    void move_item(std::string src_path, std::string dst_path);

    struct myfs_header {
        char magic[4];
        uint8_t version;
    };

private:
    BlockDeviceSimulator *blkdevsim;

    static const uint8_t CURR_VERSION = 0x03;
    static const char *MYFS_MAGIC;

    int find_inode(std::string path_str);
    void fragmentMemory(uint32_t oldEnd, uint32_t newEnd);
    uint32_t get_data_end();
};

#endif // __MYFS_H__