#include "filesystem.hpp"

CONSTEXPR HASH_TYPE HASH_SEED = 0x70BAD0BADEADBEEF;

CONSTEXPR HASH_TYPE FSPack_Hash(const char* input) {
    u32 index = 0;
    HASH_TYPE hash = HASH_SEED;

    for (index = 0; input[index] != '\0'; index++) {
        hash = (hash << ((sizeof(HASH_TYPE) * 8) / 6) + hash) + input[index];
    }

    return hash;
}

#ifndef FSPACK_ROM
CONSTEXPR SIZE_TYPE FILE_ALIGNMENT = 0x10;

extern "C" {

const char PATH_SEPARATOR[] = "/";
const s32 FILE_PAD_BYTES = 0x00000000;

size_t FSPack_Align(size_t lhs, size_t modulo) {
    if (modulo == 0) return lhs;

    s32 remainder = ABS(lhs) % modulo;
    if (remainder == 0) return lhs;

    return lhs + modulo - remainder;
}

void FileTable_Construct(FSPack_FileTable* thisx) {
    thisx->fileCount = 0;
    thisx->fileOffset = (SIZE_TYPE*)malloc(sizeof(SIZE_TYPE*));
    thisx->fileHash = (u64*)malloc(sizeof(u64*));
    thisx->filePaths = (char**)malloc(sizeof(char**));
}

void Archive_Construct(FSPack_Archive* thisx) {
    strcpy(thisx->header, "fspack2");
    thisx->size = 0;
    FileTable_Construct(&thisx->fileTable);
    thisx->archiveBuffer = malloc(sizeof(SIZE_TYPE));
}

s32 Archive_Create(FSPack_Archive* archive, const char* path, const char* path_local, void* file_buffer) {
    DirentFile* dir_file;
    DIR* dir;
    FILE* file;
    s32 old_num;
    s32 new_num;
    size_t file_size;
    size_t file_size_initial;
    char new_dir[MAX_PATH];
    char new_file_dir[MAX_PATH];

    memset(new_dir, 0, MAX_PATH);
    memset(new_file_dir, 0, MAX_PATH);

    dir = opendir(path);

    if (dir == NULLPTR) {
        printf("error: failed to open directory %s\n", path);
        return AC_RESULT_ERR_DIR;
    }

    while (dir_file = readdir(dir)) {
        // eliminate current and upper directory
        s32 valid = strcmp(dir_file->d_name, ".") != 0 && strcmp(dir_file->d_name, "..") != 0;
        if (valid) {
            printf("name: %s\n", dir_file->d_name);

            if (dir_file->d_type != DT_DIR) {
                file_size_initial = 0;

                // generate new dir, should work unless path length is > MAX_PATH
                strcpy(new_dir, path);
                strcat(new_dir, PATH_SEPARATOR);
                strcat(new_dir, dir_file->d_name);

                strcpy(new_file_dir, path_local);
                strcat(new_file_dir, PATH_SEPARATOR);
                strcat(new_file_dir, dir_file->d_name);

                file = fopen(new_dir, "r");
                if (file != NULLPTR) {
                    printf("\n\topened file %s (local %s)\n", new_dir, new_file_dir);

                    fseek(file, 0, SEEK_END);
                    file_size_initial = ftell(file);

                    if (file_size_initial == -1) {
                        printf("error: failed to read file %s\n", new_dir);
                        return AC_RESULT_ERR_FILE;
                    }

                    if (fseek(file, 0, SEEK_SET) != 0) {
                        printf("error: failed to read file %s\n", new_dir);
                        return AC_RESULT_ERR_FILE;
                    }

                    file_size = FSPack_Align(file_size_initial, FILE_ALIGNMENT);

                    // resize file_buffer to align it
                    file_buffer = realloc(file_buffer, file_size);
                    memset(((u8*)file_buffer) + file_size_initial, FILE_PAD_BYTES, file_size - file_size_initial);

                    // read the file into file_buffer
                    fread(file_buffer, sizeof(u8), file_size_initial, file);
                    fclose(file);

                    // update archive with new offsets
                    old_num = archive->fileTable.fileCount;
                    new_num = old_num + 1;
                    archive->fileTable.fileOffset = (SIZE_TYPE*)realloc(archive->fileTable.fileOffset, sizeof(SIZE_TYPE) * new_num);
                    archive->fileTable.fileHash = (HASH_TYPE*)realloc(archive->fileTable.fileHash, sizeof(HASH_TYPE) * new_num);
                    archive->fileTable.filePaths = (char**)realloc(archive->fileTable.filePaths, sizeof(char*) * new_num);

                    // update archive with new data
                    archive->fileTable.fileOffset[old_num] = archive->size;
                    archive->fileTable.fileHash[old_num] = FSPack_Hash(new_file_dir);
                    archive->fileTable.filePaths[old_num] = (char*)malloc(strlen(new_file_dir) + 1);
                    strcpy(archive->fileTable.filePaths[old_num], new_file_dir);

                    // resize and write archive buffer
                    archive->archiveBuffer = realloc(archive->archiveBuffer, archive->size + file_size + 4);
                    memcpy(((u8*)archive->archiveBuffer) + archive->size, file_buffer, file_size);
                    archive->size += file_size;

                    archive->fileTable.fileCount++;
                    printf("\tadded %s to the archive\n", new_file_dir);
                }
                else {
                    printf("error: failed to read file %s\n", new_dir);
                    return AC_RESULT_ERR_FILE;
                }

                memset(new_dir, 0, MAX_PATH);
            }
            else if (dir_file->d_type == DT_DIR) {
                // generate new dir, should work unless path length is > MAX_PATH
                strcpy(new_dir, path);
                strcat(new_dir, PATH_SEPARATOR);
                strcat(new_dir, dir_file->d_name);

                strcpy(new_file_dir, path_local);
                strcat(new_file_dir, PATH_SEPARATOR);
                strcat(new_file_dir, dir_file->d_name);

                printf("\ndelving into %s (local %s)\n\n", new_dir, new_file_dir);

                old_num = Archive_Create(archive, new_dir, new_file_dir, file_buffer);
                if (old_num != AC_RESULT_SUCCESS) {
                    closedir(dir);
                    return old_num;
                }
            }
        }
    }

    closedir(dir);

    return AC_RESULT_SUCCESS;
}

s32 Archive_Write(FSPack_Archive* archive, const char* output_file, s32 byteswap, s32 debug) {
    FILE* file = NULLPTR;
    s32 index = 0;
    s32 pad = 0;
    SIZE_TYPE file_count;
    SIZE_TYPE archive_size;

    printf("byteswapped? %d\n", byteswap);

    file = fopen(output_file, "w+");

    if (file == NULLPTR) {
        printf("error: Failed to open file %s for writing\n", output_file);

        return AC_RESULT_ERR_FILE;
    }

    printf("writing header %s, %llu files with an archive size %llX\n", archive->header, archive->fileTable.fileCount, archive->size);

    fwrite(archive->header, sizeof(u8), sizeof(archive->header), file);
    fwrite(&pad, sizeof(s32), 1, file);

    archive_size = archive->size;
    file_count = archive->fileTable.fileCount;
    if (byteswap) {
        archive_size = __builtin_bswap32(archive_size);
        file_count = __builtin_bswap32(file_count);
    }

    // write archive size and file count
    fwrite(&archive_size, sizeof(SIZE_TYPE), 1, file);
    fwrite(&file_count, sizeof(SIZE_TYPE), 1, file);

    // write offset of file hashes
    file_count = ftell(file);
    file_count += (archive->fileTable.fileCount * sizeof(SIZE_TYPE)) + sizeof(u64);
    fwrite(&file_count, sizeof(SIZE_TYPE), 1, file);

    // write the offset table
    for (index = 0; index < archive->fileTable.fileCount; index++) {
        printf("file %s has offset %X\n", archive->fileTable.filePaths[index], archive->fileTable.fileOffset[index]);

        if (byteswap) {
            archive->fileTable.fileOffset[index] = __builtin_bswap32(archive->fileTable.fileOffset[index]);
        }

        fwrite(&archive->fileTable.fileOffset[index], sizeof(SIZE_TYPE), 1, file);
    }

    // write the hash table
    fwrite(&pad, sizeof(s32), 2, file);
    for (index = 0; index < archive->fileTable.fileCount; index++) {
        printf("file %s has hash %llX\n", archive->fileTable.filePaths[index], archive->fileTable.fileHash[index]);

        if (byteswap) {
            archive->fileTable.fileHash[index] = __builtin_bswap64(archive->fileTable.fileHash[index]);
        }

        fwrite(&archive->fileTable.fileHash[index], sizeof(HASH_TYPE), 1, file);
    }

    fwrite(&pad, sizeof(s32), 1, file);
    fwrite(archive->archiveBuffer, sizeof(u8), archive->size, file);

    if (debug) {
        for (index = 0; index < archive->fileTable.fileCount; index++) {
            fwrite(archive->fileTable.filePaths[index], sizeof(u8), strlen(archive->fileTable.filePaths[index]) + 1, file);
        }
    }

    fclose(file);

    return 1;
}

}; // extern "C"

#endif