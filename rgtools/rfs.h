/*
 * rfs.h
 *
 *  Created on: May 28, 2021
 *      Author: alex9932
 */

#ifndef RFS_H_
#define RFS_H_

#include <rgtypes.h>
#include <stdio.h>

struct rg_fs_header {
    char   magic[4];
    Uint32 files;
};

struct rg_fs_file {
    char   name[256];
    Uint32 offset;
    Uint32 length;
};

struct rg_filesystem {
    FILE*        file_stream;
    rg_fs_header header;
    rg_fs_file*  files;
    Uint32*      hash_array;
};

void rfs_pack(String in, String out);
void rfs_unpack(String in, String out);

#endif /* RFS_H_ */