#include "myfs.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <vector>

const char *MyFs::MYFS_MAGIC = "MYFS";

const uint32_t HEADER_OFFSET = sizeof(MyFs::myfs_header);
const uint32_t MAX_FILES = 512;
const uint32_t ENTRY_SIZE = sizeof(INODE);
const uint32_t DATA_OFFSET = HEADER_OFFSET + MAX_FILES * ENTRY_SIZE;

// ============================================================
// CONSTRUCTOR
// ============================================================

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_)
    : blkdevsim(blkdevsim_)
{
    myfs_header header;

    blkdevsim->read(
        0,
        sizeof(header),
        (char *)&header
    );

    if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
        header.version != CURR_VERSION)
    {
        std::cout << "Did not find myfs instance on blkdev" << std::endl;
        std::cout << "Creating..." << std::endl;

        format();

        std::cout << "Finished!" << std::endl;
    }
}

// ============================================================
// FORMAT
// ============================================================

void MyFs::format()
{
    myfs_header header;

    strncpy(
        header.magic,
        MYFS_MAGIC,
        sizeof(header.magic)
    );

    header.version = CURR_VERSION;

    blkdevsim->write(
        0,
        sizeof(header),
        (const char *)&header
    );

    INODE empty_inode;
    empty_inode.file_id = 0;
    memset(empty_inode.name, 0, sizeof(empty_inode.name));
    empty_inode.size = 0;
    empty_inode.start = 0;
    empty_inode.is_dir = 0;
    memset(empty_inode.padding, 0, sizeof(empty_inode.padding));

    for (uint32_t i = 0; i < MAX_FILES; i++)
    {
        uint32_t addr = HEADER_OFFSET + i * ENTRY_SIZE;

        blkdevsim->write(
            addr,
            sizeof(INODE),
            (const char *)&empty_inode
        );
    }
}

// ============================================================
// FIND INODE
// ============================================================

int MyFs::find_inode(std::string path_str)
{
    uint32_t cursor = HEADER_OFFSET;
    uint32_t endMetadata = HEADER_OFFSET + MAX_FILES * ENTRY_SIZE;

    while (cursor < endMetadata)
    {
        INODE entry;

        blkdevsim->read(
            cursor,
            sizeof(INODE),
            (char *)&entry
        );

        if (entry.file_id != 0)
        {
            char safe_name[21] = {0};
            strncpy(safe_name, entry.name, 20);

            if (path_str == safe_name)
            {
                return cursor;
            }
        }

        cursor += ENTRY_SIZE;
    }

    return -1;
}

// ============================================================
// CREATE FILE / DIRECTORY
// ============================================================

void MyFs::create_file(std::string path_str, bool directory)
{
    if (find_inode(path_str) >= 0)
    {
        throw std::runtime_error("file or directory already exists");
    }

    if (path_str.size() > 20)
    {
        throw std::runtime_error("file name too long (max 20 chars)");
    }

    uint32_t cursor = HEADER_OFFSET;

    for (uint32_t i = 0; i < MAX_FILES; i++)
    {
        INODE inode;

        blkdevsim->read(
            cursor,
            sizeof(INODE),
            (char *)&inode
        );

        if (inode.file_id == 0)
        {
            INODE new_entry;
            new_entry.file_id = i + 1;

            memset(new_entry.name, 0, sizeof(new_entry.name));
            strncpy(new_entry.name, path_str.c_str(), 20);

            new_entry.size = 0;
            new_entry.start = 0;
            new_entry.is_dir = directory ? 1 : 0;
            memset(new_entry.padding, 0, sizeof(new_entry.padding));

            blkdevsim->write(
                cursor,
                sizeof(INODE),
                (const char *)&new_entry
            );

            return;
        }

        cursor += ENTRY_SIZE;
    }

    throw std::runtime_error("no free inode available");
}

// ============================================================
// GET CONTENT
// ============================================================

std::string MyFs::get_content(std::string path_str)
{
    int inode_addr = find_inode(path_str);

    if (inode_addr < 0)
    {
        throw std::runtime_error("no such file");
    }

    INODE inode;
    blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

    if (inode.is_dir)
    {
        throw std::runtime_error("cannot read content of a directory");
    }

    if (inode.size == 0)
    {
        return "";
    }

    std::string content(inode.size, '\0');
    blkdevsim->read(inode.start, inode.size, &content[0]);

    return content;
}

// ============================================================
// FIND END OF USED DATA
// ============================================================

uint32_t MyFs::get_data_end()
{
    uint32_t data_end = DATA_OFFSET;

    for (uint32_t i = 0; i < MAX_FILES; i++)
    {
        uint32_t inode_addr = HEADER_OFFSET + i * ENTRY_SIZE;

        INODE inode;
        blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

        if (inode.file_id != 0 && !inode.is_dir && inode.size > 0)
        {
            uint32_t end = inode.start + inode.size;
            if (end > data_end)
            {
                data_end = end;
            }
        }
    }

    return data_end;
}

// ============================================================
// FRAGMENT / SHIFT MEMORY
// ============================================================

void MyFs::fragmentMemory(uint32_t oldEnd, uint32_t newEnd)
{
    if (newEnd > oldEnd)
    {
        uint32_t shift = newEnd - oldEnd;

        for (uint32_t i = 0; i < MAX_FILES; i++)
        {
            uint32_t inode_addr = HEADER_OFFSET + i * ENTRY_SIZE;
            INODE inode;
            blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

            if (inode.file_id == 0 || inode.is_dir || inode.size == 0)
                continue;

            if (inode.start >= oldEnd)
            {
                std::string buffer(inode.size, '\0');
                blkdevsim->read(inode.start, inode.size, &buffer[0]);

                uint32_t new_start = inode.start + shift;
                blkdevsim->write(new_start, inode.size, buffer.c_str());

                inode.start = new_start;
                blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&inode);
            }
        }
    }
    else if (newEnd < oldEnd)
    {
        uint32_t shift = oldEnd - newEnd;

        for (uint32_t i = 0; i < MAX_FILES; i++)
        {
            uint32_t inode_addr = HEADER_OFFSET + i * ENTRY_SIZE;
            INODE inode;
            blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

            if (inode.file_id == 0 || inode.is_dir || inode.size == 0)
                continue;

            if (inode.start >= oldEnd)
            {
                std::string buffer(inode.size, '\0');
                blkdevsim->read(inode.start, inode.size, &buffer[0]);

                uint32_t new_start = inode.start - shift;
                blkdevsim->write(new_start, inode.size, buffer.c_str());

                inode.start = new_start;
                blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&inode);
            }
        }
    }
}

// ============================================================
// SET CONTENT
// ============================================================

void MyFs::set_content(std::string path_str, std::string content)
{
    int inode_addr = find_inode(path_str);

    if (inode_addr < 0)
    {
        throw std::runtime_error("no such file");
    }

    INODE inode;
    blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

    if (inode.is_dir)
    {
        throw std::runtime_error("cannot edit a directory");
    }

    uint32_t old_size = inode.size;
    uint32_t new_size = content.size();

    if (old_size == 0)
    {
        if (new_size == 0)
            return;

        uint32_t data_end = get_data_end();
        blkdevsim->write(data_end, new_size, content.c_str());

        inode.start = data_end;
        inode.size = new_size;
        blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&inode);
        return;
    }

    uint32_t old_start = inode.start;
    uint32_t old_end = old_start + old_size;
    uint32_t new_end = old_start + new_size;

    if (new_size == old_size)
    {
        blkdevsim->write(old_start, new_size, content.c_str());
        return;
    }

    if (new_size < old_size)
    {
        blkdevsim->write(old_start, new_size, content.c_str());
        fragmentMemory(old_end, new_end);

        inode.size = new_size;
        blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&inode);
        return;
    }

    if (new_size > old_size)
    {
        fragmentMemory(old_end, new_end);
        blkdevsim->write(old_start, new_size, content.c_str());

        inode.size = new_size;
        blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&inode);
        return;
    }
}

// ============================================================
// LIST DIRECTORY
// ============================================================

void MyFs::list_dir(std::string path_str)
{
    for (uint32_t i = 0; i < MAX_FILES; i++)
    {
        uint32_t inode_addr = HEADER_OFFSET + i * ENTRY_SIZE;

        INODE inode;
        blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

        if (inode.file_id != 0)
        {
            char name[21] = {0};
            strncpy(name, inode.name, 20);

            std::cout << std::left << std::setw(24) << name;
            if (inode.is_dir) {
                std::cout << "<DIR>";
            } else {
                std::cout << inode.size;
            }
            std::cout << std::endl;
        }
    }
}

// ============================================================
// REMOVE FILE
// ============================================================

void MyFs::remove_file(std::string path_str)
{
    int inode_addr = find_inode(path_str);

    if (inode_addr < 0)
    {
        throw std::runtime_error("no such file");
    }

    INODE inode;
    blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

    if (inode.is_dir)
    {
        throw std::runtime_error("use rmdir to delete directories");
    }

    uint32_t old_start = inode.start;
    uint32_t old_size = inode.size;

    if (old_size > 0)
    {
        uint32_t old_end = old_start + old_size;

        for (uint32_t i = 0; i < MAX_FILES; i++)
        {
            uint32_t current_inode_addr = HEADER_OFFSET + i * ENTRY_SIZE;

            INODE current_inode;
            blkdevsim->read(current_inode_addr, sizeof(INODE), (char *)&current_inode);

            if (current_inode.file_id == 0 || current_inode.is_dir || current_inode.size == 0)
            {
                continue;
            }

            if (current_inode.start >= old_end)
            {
                std::string buffer(current_inode.size, '\0');
                blkdevsim->read(current_inode.start, current_inode.size, &buffer[0]);

                uint32_t new_start = current_inode.start - old_size;
                blkdevsim->write(new_start, current_inode.size, buffer.c_str());

                current_inode.start = new_start;
                blkdevsim->write(current_inode_addr, sizeof(INODE), (const char *)&current_inode);
            }
        }
    }

    INODE empty_inode;
    empty_inode.file_id = 0;
    memset(empty_inode.name, 0, sizeof(empty_inode.name));
    empty_inode.size = 0;
    empty_inode.start = 0;
    empty_inode.is_dir = 0;
    memset(empty_inode.padding, 0, sizeof(empty_inode.padding));

    blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&empty_inode);
}

// ============================================================
// REMOVE DIRECTORY (BONUS PART B)
// ============================================================

void MyFs::remove_dir(std::string path_str)
{
    int inode_addr = find_inode(path_str);

    if (inode_addr < 0)
    {
        throw std::runtime_error("no such directory");
    }

    INODE inode;
    blkdevsim->read(inode_addr, sizeof(INODE), (char *)&inode);

    if (!inode.is_dir)
    {
        throw std::runtime_error("not a directory");
    }

    INODE empty_inode;
    empty_inode.file_id = 0;
    memset(empty_inode.name, 0, sizeof(empty_inode.name));
    empty_inode.size = 0;
    empty_inode.start = 0;
    empty_inode.is_dir = 0;
    memset(empty_inode.padding, 0, sizeof(empty_inode.padding));

    blkdevsim->write(inode_addr, sizeof(INODE), (const char *)&empty_inode);
}

// ============================================================
// MOVE / RENAME ITEM (BONUS PART B)
// ============================================================

void MyFs::move_item(std::string src_path, std::string dst_path)
{
    int src_addr = find_inode(src_path);
    if (src_addr < 0)
    {
        throw std::runtime_error("source path does not exist");
    }

    if (find_inode(dst_path) >= 0)
    {
        throw std::runtime_error("destination path already exists");
    }

    if (dst_path.size() > 20)
    {
        throw std::runtime_error("destination name too long");
    }

    INODE inode;
    blkdevsim->read(src_addr, sizeof(INODE), (char *)&inode);

    memset(inode.name, 0, sizeof(inode.name));
    strncpy(inode.name, dst_path.c_str(), 20);

    blkdevsim->write(src_addr, sizeof(INODE), (const char *)&inode);
}