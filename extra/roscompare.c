#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct FolderFiles {
    int count;
    char **names;
};

int compareStrings(const void *a, const void *b) {
    const char **str_a = (const char **)a;
    const char **str_b = (const char **)b;
    return strcmp(*str_a, *str_b);
}

struct FolderFiles listFiles(char * dirName) {
    DIR *dir;
    struct dirent *ent;
    char ** files = NULL;
    int count = 0;
    if ((dir = opendir(dirName)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0|| strcmp(ent->d_name, "..") == 0) continue;
            files = realloc(files, ++count * sizeof(char*));
            files[count-1] = malloc(strlen(ent->d_name) + 1);
            strcpy(files[count-1], ent->d_name);
        }
        closedir(dir);
    } else {
        printf("open error!\n");
        exit(1);
    }
    return (struct FolderFiles){count, files};
}

int compareFile(char* folder1, char* file1, char* folder2, char* file2) {
    char build1[256], build2[256];
    sprintf(build1, "%s%s", folder1, file1);
    sprintf(build2, "%s%s", folder2, file2);
    FILE * f1 = fopen(build1, "rb");
    if (f1 == NULL) return -1;
    FILE * f2 = fopen(build2, "rb");
    if (f2 == NULL) {
        fclose(f1);
        return -2;
    }

    fseek(f1, 0, SEEK_END);
    fseek(f2, 0, SEEK_END);
    const long size1 = ftell(f1), size2 = ftell(f2);
    fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_SET);

    if (size1 != size2) {
        fclose(f1);
        fclose(f2);
        return -3;
    }

    void * d1 = malloc(size1), * d2 = malloc(size2);
    fread(d1, size1, 1, f1);
    fread(d2, size2, 1, f2);

    fclose(f1);
    fclose(f2);

    if (memcmp(d1, d2, size1) != 0) {
        free(d1);
        free(d2);
        return -4;
    }
    free(d1);
    free(d2);
    return 0;
}

int main() {
    char  *fn1 = "ofw/", *fn2 = "evilnat/";
    struct FolderFiles folder1 = listFiles(fn1);
    struct FolderFiles folder2 = listFiles(fn2);

    if (folder1.count > 0) {
        qsort(folder1.names, folder1.count, sizeof(char *), compareStrings);
    }

    printf("%s: %d | %s: %d\n", fn1, folder1.count, fn2, folder2.count);
    printf("Base: %s\n", fn1);
    int same = 0;
    for (int i = 0; i < folder1.count; i++) {
        int res = compareFile(fn1, folder1.names[i], fn2, folder1.names[i]);
        if (res == 0) same++;
        else printf("%s differs with: %d\n", folder1.names[i], res);
    }

    printf("Same: %d\n", same);
}
