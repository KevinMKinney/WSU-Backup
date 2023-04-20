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

#define BACKUPDIRNAME ".backup"
#define BACKUPSUFFIX ".bak"
#define DEBUG 0
#define FILENAMEBUF 100 // Maybe use FILENAME_MAX
#define BUFSIZE 512

// List data structure that contains the files that need to be backed up or restored.
typedef struct ListNode{
    pthread_t t;
    struct ListNode *next;
}node;

// The data that is passed to each thread (used in conjunction with the list).
typedef struct thread_data{
    char* curDir;
    char* fileName;
    int index;
} td;

// global variables for threads
int threadCount;
int backupExistsFlag = 1;
int restoreFlag = 0;
int fileCount = 0;
int fileSize = 0;
/**
 * insertThread = Appends a new thread node to the list. The node's data is provided as input.
 * 
 *      d - The thread pointer to be inserted
 *      head - The node at the front of the list
 *      tail - The node at the end of the list
 **/
void insertThread(pthread_t d, node **head, node **tail){
    node *n = (node *) malloc(sizeof(node));
    if(n == NULL) exit(1);
    n->t = d;
    n->next = NULL;

    if(*head == NULL){
        *head = n;
        *tail = n;
    }else{
        
        (*tail)->next = n;
        *tail = n;    
    }
    
}

/**
 * backUpFile = This is how the backing of files is done. Where, after creating a 
 * thread to run this function, it takes the source file, and then make a backup 
 * file of it, then put it's contents in it.
 * 
 *      param - The thread data
 **/
void * backUpFile(void *param){
    //set variables up
    FILE *fp = NULL, *fp2 = NULL;
    errno = 0;
    td *myTD = (td *) param;
    int index = myTD->index;
    char* dir = myTD->fileName;
    int bytesRead = 0;
    char * str = malloc(PATH_MAX * sizeof(char));
    char * sourceStr = malloc(PATH_MAX * sizeof(char));
    char readC;
    //malloc check
    if(str == NULL || sourceStr == NULL){
        printf("Memory creation issue with file name\n");
        exit(1);
    }

    //Get the name of the new file
    strncpy(str, myTD->curDir, PATH_MAX);
    strncat(str, "/", 2);
    strncat(str, BACKUPDIRNAME, 8);
    strncat(str, "/", 2);
    strncat(str, dir, FILENAMEBUF);
    //char *b = ".bak";
    strncat(str, BACKUPSUFFIX, 5);
    if(DEBUG == 1) printf("File created: %s\n", str);

    strncpy(sourceStr, myTD->curDir, PATH_MAX);
    strncat(sourceStr, "/", 2);
    strncat(sourceStr, dir, PATH_MAX);
    
    
    if(!restoreFlag){
        //Open up the files, with error check
        fp = fopen(sourceStr, "r");
        fp2 = fopen(str, "w"); //write to dest file
        if ( fp == NULL || fp2 == NULL) {
		    fprintf(stderr, "Cannot open file %s or %s to read/write\n", sourceStr, str);
		    free(str);
		    exit(1);
	    }

        //The actually coping is done here, char by char
        while(!feof(fp)){
            readC = fgetc(fp);
            if(feof(fp)) break;
            fputc(readC, fp2);
            bytesRead += 1;//count how many bytes have been copied
            
        }
        memset(str, '\0', sizeof(char) * PATH_MAX);
        strncpy(str, dir, PATH_MAX);
        strncat(str, BACKUPSUFFIX, 5);

        printf("[thread %d] Copied %d bytes from %s to %s\n", index, bytesRead, dir, str);
    
    }else{
        //Open up the files, with error check
        fp = fopen(sourceStr, "w");
        fp2 = fopen(str, "r"); //write to dest file
        if ( fp == NULL || fp2 == NULL) {
		    fprintf(stderr, "Cannot open file %s or %s to read/write\n", sourceStr, str);
		    free(str);
		    exit(1);
	    }
        //Read charcater by character backup file, then write to file
        while(!feof(fp2)){  
            readC = fgetc(fp2);
            if(feof(fp2)) break;
            fputc(readC, fp);
            bytesRead+= 1; 
        }
        memset(str, '\0', sizeof(char) * PATH_MAX);
        strncpy(str, dir, PATH_MAX);
        strncat(str, BACKUPSUFFIX, 5);
        
        printf("[thread %d] Copied %d bytes from %s to %s\n", index, bytesRead, str, dir);
        
    } 
    
    //Close the files 
    if(fclose(fp) != 0 || fclose(fp2)){
        printf("Cannot close file %s or %s\n", dir, str);
        exit(1);
    }
    free(str);
    free(myTD->curDir);
    free(myTD);
    free(sourceStr);
    if(restoreFlag) free(dir);
    return NULL;
}

/**
 * getBackup = The function tries to find the .backup directory (or whatever
 * the backup directory is named based on the macro) and returns its corresponding 
 * DIR pointer. If its not found during this process, a new .backup is made.
 * 
 *      curDirName - The absolute path of the directory the file is located in
 **/
DIR* getBackup(char* curDirName) {

    char* backDirName = calloc(PATH_MAX, sizeof(char));

    //calloc check
    if(backDirName == NULL) {
        printf("Memory creation issue with backup directory name\n");
        exit(1);
    }

    // set backDirName to be the absolute path to the backup directory
    backDirName = strncpy(backDirName, curDirName, PATH_MAX);
    backDirName = strncat(backDirName, "/", PATH_MAX);
    backDirName = strncat(backDirName, BACKUPDIRNAME, PATH_MAX);
    backupExistsFlag = 1;
    // get the backup directory
    DIR* backDir = opendir(backDirName);
    
    if (backDir == NULL) {
        backupExistsFlag = 0;
        if (errno != ENOENT) {
            printf("%s\n", strerror(errno));
            exit(1);
        }

        // could  not find directory, so make a new one
        mkdir(backDirName, S_IRWXU);

        backDir = opendir(backDirName);

        if (backDir == NULL) {
            printf("%s\n", strerror(errno));
            exit(1);
        }
    }

    free(backDirName);
    return backDir;
}

/**
 * restoreDir = This is how the restoration of files is done. Where, after creating a 
 * thread to run this function, it takes the backup file, and then copies its contents
 * to the source file.
 * 
 *      param - The thread data
 **/
void restoreDir(char* curDirName){

    if (DEBUG != 0) { printf("cur: %s\n", curDirName); }

    errno = 0;
    DIR* curDir = opendir(curDirName);

    if (curDir == NULL) {
        printf("%s\n", strerror(errno));
        exit(1);
    }

    struct dirent* dir = NULL;
    DIR* backUpDir = getBackup(curDirName);
    node *head = NULL, *tail = NULL;
    char* newDirName = calloc(PATH_MAX, sizeof(char));
    char* backDirName = calloc(PATH_MAX, sizeof(char));
    
    if(newDirName == NULL || backDirName == NULL) exit(1);
    
    while ((dir = readdir(backUpDir)) != NULL) {
        struct stat st;
                
        // Ignore these directories
        if (!strcmp(dir->d_name, ".") || 
            !strcmp(dir->d_name, "..") || 
            !strcmp(dir->d_name, ".git") || 
            !strcmp(dir->d_name, BACKUPDIRNAME)) {
            continue;
        }

        // set backDirName to be the absolute path to the backup file
        memset(backDirName, '\0', sizeof(char) * PATH_MAX);
        strncpy(backDirName, curDirName, PATH_MAX);
        strncat(backDirName, "/", 2);
        strncat(backDirName, BACKUPDIRNAME, 8);
        strncat(backDirName, "/", PATH_MAX);
        strncat(backDirName, dir->d_name, PATH_MAX);

        if (stat(backDirName, &st) < 0) {
            // could not get information about file
            if (DEBUG != 0) { printf("could not read: %s\n", dir->d_name); }
            if (errno != 0) {
                printf("%s\n", strerror(errno));
                exit(1);
            }
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            char* dName = calloc(PATH_MAX, sizeof(char));
            if(dName == NULL) exit(1);
            memset(newDirName, '\0', sizeof(char) * PATH_MAX);
            strncpy(dName, dir->d_name, strlen(dir->d_name)-4);
            newDirName = strncpy(newDirName, curDirName, PATH_MAX);
            newDirName = strncat(newDirName, "/", PATH_MAX);
            newDirName = strncat(newDirName, dName, PATH_MAX);

            printf("[thread %d] Restoring %s\n", threadCount, dName);
            
            struct stat st2;
            if (stat(newDirName, &st2) < 0) {
            // if file does not exist in current working directory
                if (DEBUG != 0) { printf("could not read: %s\n", dName); }
                if (errno != 0 && errno != 2) {
                    printf("%s\n", strerror(errno));
                    exit(1);
                }
                //File exist in backup, but not in cwd, so add it
                fileCount++;
                pthread_t t;
                td *myTd = malloc(sizeof(td));
                if(myTd == NULL) exit(1);
                myTd->index = threadCount;
                myTd->fileName = dName;
                myTd->curDir = calloc(PATH_MAX, sizeof(char));
                if(myTd->curDir == NULL) exit(1);
                myTd->curDir = strncpy(myTd->curDir, curDirName, PATH_MAX);
        
                pthread_create(&t, NULL, backUpFile, (void *)myTd);//(void *)n->data);
                insertThread(t, &head, &tail);
                threadCount++;
                fileSize += st.st_size;
            }
            
            else if (st.st_mtime < st2.st_mtime || !strcmp(dir->d_name, "BackItUp.bak")){
                printf("[thread %d] %s is already the most current version\n", threadCount, dName);
                threadCount++;
                free(dName);
            }
            else{
                //Need to restore the file, not most current version 
                fileCount++;
                pthread_t t;
                td *myTd = malloc(sizeof(td));
                if(myTd == NULL) exit(1);
                myTd->index = threadCount;
                myTd->fileName = dName;
                myTd->curDir = calloc(PATH_MAX, sizeof(char));
                if(myTd->curDir == NULL) exit(1);
                myTd->curDir = strncpy(myTd->curDir, curDirName, PATH_MAX);
        
                pthread_create(&t, NULL, backUpFile, (void *)myTd);//(void *)n->data);
                insertThread(t, &head, &tail);
                threadCount++;
                fileSize += st.st_size;
                
            }
            
        }
    }    

    //Go through the subdirectories and restore their backup files
    while ((dir = readdir(curDir)) != NULL) {
    
        struct stat st;
       
        // Ignore these directories
        if (!strcmp(dir->d_name, ".") || 
            !strcmp(dir->d_name, "..") || 
            !strcmp(dir->d_name, ".git") || 
            !strcmp(dir->d_name, BACKUPDIRNAME)) {
            continue;
        }

        // set newDirName to be the absolute file path name
        memset(newDirName, '\0', sizeof(char) * PATH_MAX);
        newDirName = strncpy(newDirName, curDirName, PATH_MAX);
        newDirName = strncat(newDirName, "/", PATH_MAX);
        newDirName = strncat(newDirName, dir->d_name, PATH_MAX);

        //If file doesn't exist or able to read/write from it
        if (stat(newDirName, &st) < 0) {
            // could not get information about file
            if (DEBUG != 0) { printf("could not read: %s\n", dir->d_name); }
            if (errno != 0) {
                printf("%s\n", strerror(errno));
                exit(1);
            }
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            // it is a directory,
            // recursively call this function to get backups
            if (DEBUG != 0) {
                printf("Dir: %s\n", dir->d_name);
            }
            restoreDir(dir->d_name);
        }
    }
    //This is where the waiting of restoring threads is done
    node *n = head, *temp = NULL;
    
    while(n != NULL){//Join all threads
        pthread_join(n->t, NULL);
        temp = n->next;
        free(n);
        n = temp;
    }
    
    // reached end of directory
    free(newDirName);
    free(backDirName);
    if(closedir(curDir) || closedir(backUpDir)){
        printf("%s\n", strerror(errno));
        exit(1);
    }
}

/**
 * iterateDir = From the cwd, recursivly iterate (Depth first) through all 
 * regular files and directories. If it is a regular file, append to a list
 * which is then passed to threads for backing it up or restoring it after
 * iterating through the current directory. If it is a directory, recusively
 * call this function on it.
 * 
 *      curDirName - The absolute path of the directory the file is located in
 **/
void iterateDir(char* curDirName) {
    if (DEBUG != 0) { printf("cur: %s\n", curDirName); }

    errno = 0;
    DIR* curDir = opendir(curDirName);

    if (curDir == NULL) {
        printf("%s\n", strerror(errno));
        exit(1);
    }

    struct dirent* dir = NULL;
    struct dirent* dir2 = NULL;
    DIR* backUpDir = getBackup(curDirName);
    int oldBackup = backupExistsFlag; 
    node *head = NULL, *tail = NULL;
    char* newDirName = calloc(PATH_MAX, sizeof(char));
    char* backDirName = calloc(PATH_MAX, sizeof(char));

    //calloc check
    if(newDirName == NULL || backDirName == NULL) {
        printf("Memory creation issue with backup directory name\n");
        exit(1);
    }

    while ((dir = readdir(curDir)) != NULL) {
        struct stat st;
        
        // Ignore these directories
        if (!strcmp(dir->d_name, ".") || 
            !strcmp(dir->d_name, "..") || 
            !strcmp(dir->d_name, ".git") || 
            !strcmp(dir->d_name, BACKUPDIRNAME)) {
            continue;
        }

        // set newDirName to be the absolute file path name
        memset(newDirName, '\0', sizeof(char) * PATH_MAX);
        newDirName = strncpy(newDirName, curDirName, PATH_MAX);
        newDirName = strncat(newDirName, "/", PATH_MAX);
        newDirName = strncat(newDirName, dir->d_name, PATH_MAX);

        //printf("DDNAME: %s\n", dir->d_name);
        if (stat(newDirName, &st) < 0) {
            // could not get information about file
            if (DEBUG != 0) { printf("could not read: %s\n", dir->d_name); }
            if (errno != 0) {
                printf("%s\n", strerror(errno));
                exit(1);
            }
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            // it is a regular file,
            // add onto the list for it to be backed up
            if (DEBUG != 0) { printf("File: %s\n", dir->d_name); }
            // checking to see if the file already has a backup
            if(oldBackup == 1){
                
                // set newDirName to be the backup file name (reusing the variable)
                memset(newDirName, '\0', sizeof(char) * PATH_MAX);
                newDirName = strncpy(newDirName, dir->d_name, PATH_MAX);
                newDirName = strncat(newDirName, BACKUPSUFFIX, 5);
                struct stat st2;

                // set backDirName to be the absolute path to the .backup in the cwd
                memset(backDirName, '\0', sizeof(char) * PATH_MAX);
                strncpy(backDirName, curDirName, PATH_MAX);
                strncat(backDirName, "/", 2);
                strncat(backDirName, BACKUPDIRNAME, 8);
                strncat(backDirName, "/", PATH_MAX);
                strncat(backDirName, newDirName, PATH_MAX);
                
                printf("[thread %d] Backing up %s\n", threadCount, dir->d_name);
                if (stat(backDirName, &st2) < 0) {
                    // File does not exist, or there are some issues with it
                    if (DEBUG != 0) { printf("could not read: %s\n", dir2->d_name); }
                    if (errno != 0 && errno != 2) {
                        printf("%s\n", strerror(errno));
                        exit(1);
                    }
                    fileCount++;
                 
                    pthread_t t;
                    td *myTd = malloc(sizeof(td));
                    if(myTd == NULL) exit(1);
                    myTd->index = threadCount;
                    myTd->fileName = dir->d_name;
                    myTd->curDir = calloc(PATH_MAX, sizeof(char));
                    if(myTd->curDir == NULL) exit(1);
                    myTd->curDir = strncpy(myTd->curDir, curDirName, PATH_MAX);
                    
                    pthread_create(&t, NULL, backUpFile, (void *)myTd);//(void *)n->data);
                    insertThread(t, &head, &tail);
                    threadCount++;
                    fileSize += st.st_size;
                }
                
                else if (st.st_mtime <= st2.st_mtime) {
                    // file's backup is up-to-date, so don't backup
                    //bu = 0;
                    printf("[thread %d] %s does not need backing up\n", threadCount, dir->d_name);
                    threadCount++;
                    
                }else{
                    // found a file needing to be changed, put it in the list
                    fileCount++;
                    
                    pthread_t t;
                    td *myTd = malloc(sizeof(td));
                    if(myTd == NULL) exit(1);
                    myTd->index = threadCount;
                    myTd->fileName = dir->d_name;
                    myTd->curDir = calloc(PATH_MAX, sizeof(char));
                    if(myTd->curDir == NULL) exit(1);
                    myTd->curDir = strncpy(myTd->curDir, curDirName, PATH_MAX);
                
                    printf("[thread %d] WARNING overwriting %s\n", threadCount, newDirName);
        
                    pthread_create(&t, NULL, backUpFile, (void *)myTd);//(void *)n->data);
                    insertThread(t, &head, &tail);
                    threadCount++;
                    fileSize += st.st_size;
                }
            }
            else{
                fileCount++;
                
                pthread_t t;
                td *myTd = malloc(sizeof(td));
                if(myTd == NULL) exit(1);
                myTd->index = threadCount;
                myTd->fileName = dir->d_name;
                myTd->curDir = calloc(PATH_MAX, sizeof(char));
                if(myTd->curDir == NULL) exit(1);
                myTd->curDir = strncpy(myTd->curDir, curDirName, PATH_MAX);
                
                printf("[thread %d] Backing up %s\n", threadCount, dir->d_name);
        
                pthread_create(&t, NULL, backUpFile, (void *)myTd);//(void *)n->data);
                insertThread(t, &head, &tail);
                threadCount++;
                fileSize += st.st_size;
            }
        }


        if (S_ISDIR(st.st_mode)) {
            // it is a directory,
            // recursively call this function to read it
            if (DEBUG != 0) {
                printf("Dir: %s\n", dir->d_name);
            }
            iterateDir(dir->d_name);
        }
    }
    node *n = head, *temp = NULL;
    //This is where the waiting for files to be executed is done
    while(n != NULL){
        pthread_join(n->t, NULL);
        temp = n->next;
        free(n);
        n = temp;
    }
    
    free(newDirName);
    free(backDirName);
    
    //close the directories once we are done with them
    if(closedir(curDir) || closedir(backUpDir)){
        printf("%s\n", strerror(errno));
        exit(1);
    }
    
    
}

int main(int argc, char const *argv[]) {
    
    threadCount = 1;
    char curDir[PATH_MAX];
    if (getcwd(curDir, sizeof(curDir)) == NULL) {
        printf("%s\n", strerror(errno));
        exit(1);
    }

    // check if executable was ran with a -r flag
    if(argc == 2 && !strcmp(argv[1], "-r")){
        restoreFlag = 1;
        restoreDir(curDir);
    }
    else if(argc == 1){
        iterateDir(curDir);
    }
    //Once everything is joined in the functions, print the file count and overall bytes read
    printf("Successfully copied %d files (%d) \n", fileCount, fileSize);
    return 0;
}
