#ifndef FSPACK_FILESYSTEM_H
#define FSPACK_FILESYSTEM_H

#include "inttypes.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef u64 HASH_TYPE;

/* Filesystem structure:
 * "fspack2"
 * padding (4 bytes)
 * archive size
 * file count
 * offset of hash table
 *  file offset table (file count * sizeof SIZE_TYPE bytes)
 *  padding (8 bytes)
 *  file path hash table (file count * sizeof HASH_TYPE bytes)
 * padding (4 bytes)
 * archive
 * if debug enabled, path strings
*/
typedef struct {
    SIZE_TYPE fileCount;
    SIZE_TYPE* fileOffset;
    HASH_TYPE* fileHash;
    char** filePaths;
} FSPack_FileTable;

typedef struct {
    char header[8];
    FSPack_FileTable fileTable;
    SIZE_TYPE size;
    void* archiveBuffer;
} FSPack_Archive;

enum {
    AC_RESULT_SUCCESS = 1,
    AC_RESULT_ERR_DIR,
    AC_RESULT_ERR_FILE
};

CONSTEXPR HASH_TYPE FSPack_Hash(const char* input);

#ifdef __cplusplus
extern "C" {
#endif
void FileTable_Construct(FSPack_FileTable* thisx);
void Archive_Construct(FSPack_Archive* thisx);
s32 Archive_Create(FSPack_Archive* archive, const char* path, const char* path_local, void* file_buffer);
s32 Archive_Write(FSPack_Archive* archive, const char* output_file, s32 byteswap, s32 debug);
#ifdef __cplusplus
}; // extern "C"
#endif

#endif
