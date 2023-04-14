#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define DEBUG 1
#define BACKUPDIRNAME "backup"
#define FILENAMEBUF 100

DIR* getBackup() {
    // get the backup directory
    DIR* backDir = opendir(BACKUPDIRNAME);

    if (backDir == NULL) {
        if (errno != ENOENT) {
            perror(strerror(errno));
            exit(1);
        }

        // could not find directory, so make a new one
        mkdir(BACKUPDIRNAME, S_IRWXU);

        backDir = opendir(BACKUPDIRNAME);
    }

    return backDir;
}

void iterateDir(char* curDirName) {

    errno = 0;
    //DIR* backDir = getBackup();
    DIR* curDir = opendir(curDirName);
    struct dirent* dir = NULL;

    while ((dir = readdir(curDir)) != NULL) {
        struct stat st;

        if (stat(dir->d_name, &st) < 0) {
            // could not get information about file
            if (DEBUG != 0) {
                printf("could not read: %s\n", dir->d_name);
            }

            if (errno != 0) {
                printf("%s\n", strerror(errno));
                exit(1);
            }
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            // it is a regular file,
            // do a backup thing!
            if (DEBUG != 0) {
                printf("File: %s\n", dir->d_name);
            }
        }

        if (S_ISDIR(st.st_mode)) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
                continue;
            }

            // it is a directory,
            // recursively call this function to read it
            if (DEBUG != 0) {
                printf("Dir: %s\n", dir->d_name);
            }
            //iterateDir(dir->d_name);
        }
    }

    if (errno != 0) {
        printf("%s\n", strerror(errno));
        exit(1);
    }

    // reached end of directory

}

int main(int argc, char const *argv[]) {
    
    //TODO: make the "-r" option flag (for restoring files)

    char curDir[PATH_MAX];
    if (getcwd(curDir, sizeof(curDir)) == NULL) {
        printf("%s\n", strerror(errno));
        exit(1);
    }
    iterateDir(curDir);

    return 0;
}
