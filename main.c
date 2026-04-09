#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct {
    void * ptr;
    long len;
} FileData;

#pragma pack(push, 1)

struct ROSHeader {
    uint64_t unk0;
    uint64_t len0;
    uint32_t unk1;
    uint32_t entries;
    uint64_t len1;
} __attribute__((packed));

struct ROSHeaderPatch {
    uint32_t unk1;
    uint32_t entries;
    uint64_t len1;
} __attribute__((packed));

struct ROSFileEntry {
    uint64_t offset;
    uint64_t size;
    uint8_t name[32];
} __attribute__((packed));

#pragma pack(pop)

FileData loadFile() {
    FILE * f = fopen("ros.bin", "rb");
    if (f == NULL) {
        printf("Error opening file\n");
        return (FileData){NULL, 0};
    }

    fseek(f, 0, SEEK_END);
    const long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // ReSharper disable once CppDFAMemoryLeak
    void * fdata = malloc(fsize);
    fread(fdata, fsize, 1, f);
    fclose(f);

    // ReSharper disable once CppDFAMemoryLeak
    return (FileData){fdata, fsize};
}

char isValidFileName(const char* str, int max) {
    for (int i = 0; i < max; i++) {
        if (str[i] < 'a' || str[i] > 'z') {
            if (str[i] < '0' || str[i] > '9') {
                if (str[i] != '.' && str[i] != '_') {
                    return 0;
                }
            }
        }
    }
    return 1;
}

char isHeaderless(const FileData *data) {
    const struct ROSFileEntry * tryParse = data->ptr;
    return isValidFileName((char*) tryParse->name, 5);
}

int main() {
    const FileData result = loadFile();
    FileData data = result;
    if (data.ptr == NULL) return 1;
    const char headerless = isHeaderless(&data);
    printf("File is %s\n", headerless ? "headerless" : "not headerless");
    struct ROSHeaderPatch header = {};

    if (headerless == 0) {
        header = *(struct ROSHeaderPatch *)data.ptr;
        data.ptr += sizeof(struct ROSHeaderPatch);
        data.len -= sizeof(struct ROSHeaderPatch);
    }

    int entriesCount = __builtin_bswap32(header.entries);
    if (headerless == 1) {
        const int maxEntries = data.len / sizeof(struct ROSFileEntry);
        entriesCount = 0;
        for (int i = 0; i < maxEntries; i++) {
            const struct ROSFileEntry entry = ((struct ROSFileEntry *)data.ptr)[i];
            if (isValidFileName((char*) entry.name, 3)) {
                entriesCount++;
            } else {
                break;
            }
        }
    }

    printf("Entries count: %d\n", entriesCount);
    if (entriesCount > 0 && headerless == 0) {
        _wmkdir(L"output");
    }
    for (int i = 0; i < entriesCount; i++) {
        const struct ROSFileEntry * entry = (struct ROSFileEntry *)data.ptr + i;
        printf("Entry %d: name=%s, offset=%llu, size=%llu\n", i, (char*) entry->name, __builtin_bswap64(entry->offset), __builtin_bswap64(entry->size));

        if (headerless == 0) {
            char fname[64];
            snprintf(fname, sizeof(fname), "output/%s", (char*) entry->name);
            FILE * f = fopen(fname, "wb");
            if (f == NULL) {
                printf("Error opening file %s\n", fname);
                continue;
            }

            fwrite(result.ptr + __builtin_bswap64(entry->offset), 1, __builtin_bswap64(entry->size), f);
            fclose(f);
        }
    }

    free(result.ptr);
    return 0;
}
