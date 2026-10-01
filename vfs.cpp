#include "vfs.h"
#include "myfs.h"

#include <iostream>
#include <sstream>
#include <vector>

const std::string FS_NAME = "myfs";

const std::string LIST_CMD = "ls";
const std::string CONTENT_CMD = "cat";
const std::string CREATE_FILE_CMD = "touch";
const std::string EDIT_CMD = "edit";
const std::string REMOVE_CMD = "rm";
const std::string MKDIR_CMD = "mkdir";
const std::string RMDIR_CMD = "rmdir";
const std::string MOVE_CMD = "mv";
const std::string HELP_CMD = "help";
const std::string EXIT_CMD = "exit";

const std::string HELP_STRING = "The following commands are supported: \n"
    + LIST_CMD + " [<directory>] - list directory content. \n"
    + CONTENT_CMD + " <path> - show file content. \n"
    + CREATE_FILE_CMD + " <path> - create empty file. \n"
    + EDIT_CMD + " <path> - re-set file content. \n"
    + REMOVE_CMD + " <path> - remove file. \n"
    + MKDIR_CMD + " <path> - create a directory. \n"
    + RMDIR_CMD + " <path> - remove a directory and its contents. \n"
    + MOVE_CMD + " <src> <dst> - move or rename a file or directory. \n"
    + HELP_CMD + " - show this help message. \n"
    + EXIT_CMD + " - gracefully exit. \n";

std::vector<std::string> split_cmd(std::string cmd) {
    std::stringstream ss(cmd);
    std::string part;
    std::vector<std::string> ans;

    while (std::getline(ss, part, ' ')) {
        if (!part.empty()) {
            ans.push_back(part);
        }
    }

    return ans;
}

void run_vfs(MyFs &fs) {
    std::cout << "Welcome to " << FS_NAME << std::endl;
    std::cout << "To get help, please type 'help' on the prompt below." << std::endl;
    std::cout << std::endl;

    bool exit = false;
    while (!exit) {
        try {
            std::string cmdline;
            std::cout << FS_NAME << "$ ";
            if (!std::getline(std::cin, cmdline, '\n')) {
                break;
            }

            if (cmdline.empty())
                continue;

            std::vector<std::string> cmd = split_cmd(cmdline);
            if (cmd.empty())
                continue;

            if (cmd[0] == EXIT_CMD) {
                exit = true;
            } else if (cmd[0] == HELP_CMD) {
                std::cout << HELP_STRING;
            } else if (cmd[0] == LIST_CMD) {
                if (cmd.size() == 1) {
                    fs.list_dir("/");
                } else {
                    fs.list_dir(cmd[1]);
                }
            } else if (cmd[0] == CREATE_FILE_CMD) {
                if (cmd.size() < 2) {
                    std::cout << "Usage: touch <path>" << std::endl;
                } else {
                    fs.create_file(cmd[1], false);
                }
            } else if (cmd[0] == CONTENT_CMD) {
                if (cmd.size() < 2) {
                    std::cout << "Usage: cat <path>" << std::endl;
                } else {
                    std::string content = fs.get_content(cmd[1]);
                    std::cout << content << std::endl;
                }
            } else if (cmd[0] == EDIT_CMD) {
                if (cmd.size() < 2) {
                    std::cout << "Usage: edit <path>" << std::endl;
                } else {
                    std::cout << "Enter new file content" << std::endl;
                    std::string content;
                    std::getline(std::cin, content);
                    fs.set_content(cmd[1], content);
                }
            } else if (cmd[0] == REMOVE_CMD) {
                if (cmd.size() < 2) {
                    std::cout << "Usage: rm <path>" << std::endl;
                } else {
                    fs.remove_file(cmd[1]);
                }
            } else if (cmd[0] == MKDIR_CMD) {
                if (cmd.size() < 2) {
                    std::cout << "Usage: mkdir <path>" << std::endl;
                } else {
                    fs.create_file(cmd[1], true);
                }
            } else if (cmd[0] == RMDIR_CMD) {
                if (cmd.size() < 2) {
                    std::cout << "Usage: rmdir <path>" << std::endl;
                } else {
                    fs.remove_dir(cmd[1]);
                }
            } else if (cmd[0] == MOVE_CMD) {
                if (cmd.size() < 3) {
                    std::cout << "Usage: mv <src> <dst>" << std::endl;
                } else {
                    fs.move_item(cmd[1], cmd[2]);
                }
            } else {
                std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
            }
        } catch (const std::exception &e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}