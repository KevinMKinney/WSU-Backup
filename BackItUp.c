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

#define BACKUPDIRNAME ".backup/"
#define FILENAMEBUF 100

void fileRead(struct dirent* dir){

    char * str = malloc((FILENAMEBUF + 4) * sizeof(char));
    if(str == NULL){
        printf("Memory creation issue with file name\n");
        exit(1);
    }
    strncpy(str, dir->d_name, FILENAMEBUF);
    char *b = ".bak";
    strncat(str, b, 4);
    printf("File: %s\n", str);
    free(str);
}

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
    DIR* backUpDir = getBackup();
    while ((dir = readdir(curDir)) != NULL) {
        struct stat st;

        if (stat(dir->d_name, &st) < 0) {
            // could not get information about file

            printf("could not read: %s\n", dir->d_name);
            if (errno != 0) {
                perror(strerror(errno));
                exit(1);
            }
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            // it is a regular file,
            // do a backup thing!
            fileRead(dir);
        }

        if (S_ISDIR(st.st_mode)) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") || !strcmp(dir->d_name, ".backup")) {
                // maybe should also include ".git"
                continue;
            }

            // it is a directory,
            // recursively call this function to read it
            printf("Dir: %s\n", dir->d_name);
        }
    }

   /* if (errno != 0) {
        perror(strerror(errno));
        exit(1);
    }*/
    closedir(curDir);
    closedir(backUpDir);
    // reached end of directory

}

int main(int argc, char const *argv[]) {
    
    //TODO: make the "-r" option flag (for restoring files)

    char curDir[PATH_MAX];
    if (getcwd(curDir, sizeof(curDir)) == NULL) {
        perror(strerror(errno));
        exit(1);
    }
    iterateDir(curDir);

    return 0;
}
