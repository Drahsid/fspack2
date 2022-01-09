#include "inttypes.h"
#include "filesystem.hpp"

s32 main(s32 argc, char* argv[]) {
    FSPack_Archive archive;
    s32 byteswap = 0;
    s32 debug = 0;
    s32 index = 0;
    char* directory = NULLPTR;
    char* output_file = NULLPTR;
    void* file_buffer = NULLPTR;
    char directory_local[MAX_PATH];

    memset(directory_local, '\0', MAX_PATH);

    for (index = 0; index < argc; index++) {
        if (strcmp(argv[index], "-d") == 0 || strcmp(argv[index], "-D") == 0 || strcmp(argv[index], "--directory") == 0) {
            directory = argv[index + 1];
        }

        if (strcmp(argv[index], "-o") == 0 || strcmp(argv[index], "-O") == 0 || strcmp(argv[index], "--outputdir") == 0) {
            output_file = argv[index + 1];
        }

        if (strcmp(argv[index], "-b") == 0 || strcmp(argv[index], "-B") == 0|| strcmp(argv[index], "--byteswap") == 0) {
            byteswap = 1;
        }

        if (strcmp(argv[index], "-g") == 0 || strcmp(argv[index], "-G") == 0|| strcmp(argv[index], "--debug") == 0) {
            debug = 1;
        }
    }

    if (directory == NULLPTR) {
        printf("error: no input directory! use `-d [dir]` to choose a directory!\n");
        return 1;
    }

    if (output_file == NULLPTR) {
        printf("error: no output file! use -o [file] to choose a file!\n");
        return 1;
    }

    printf("input directory: %s\n", directory);

    Archive_Construct(&archive);
    file_buffer = malloc(0x8000);

    printf("creating archive\n");
    Archive_Create(&archive, directory, directory_local, file_buffer);

    printf("writing archive\n");
    Archive_Write(&archive, output_file, byteswap, debug);
    printf("fspack finished\n");

    free(archive.archiveBuffer);
    
    for (index = 0; index < archive.fileTable.fileCount; index++) {
        free(archive.fileTable.filePaths[index]);
    }

    free(archive.fileTable.filePaths);
    free(archive.fileTable.fileHash);
    free(archive.fileTable.fileOffset);
    free(file_buffer);

    return 0;
}

