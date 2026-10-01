#include "blkdev.h"
#include "myfs.h"
#include "vfs.h"

#include <iostream>


int main(int argc, char **argv) {

	if (argc != 2) {
		std::cerr << "Please provide the file to operate on" << std::endl;
		return -1;
	}

	BlockDeviceSimulator *blkdevptr = new BlockDeviceSimulator(argv[1]);
	MyFs myfs(blkdevptr);

	run_vfs(myfs);
}
