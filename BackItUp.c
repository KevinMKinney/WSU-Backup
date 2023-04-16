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
#define DEBUG 0
#define FILENAMEBUF 100 // Maybe use FILENAME_MAX
#define BUFSIZE 256

typedef struct ListNode{
    struct dirent* data;
    char* curDir;
    struct ListNode *next;
}node;

typedef struct thread_data{
    char* curDir;
    struct dirent* d;
    int index;
} td;

void insert(struct dirent *d, char* curDirName, node **head, node **tail){
    node *n = (node *) malloc(sizeof(node));
    if(n == NULL) exit(1);
    n->data = d;
    n->curDir = calloc(PATH_MAX, sizeof(char));
    strncpy(n->curDir, curDirName, PATH_MAX);
    n->next = NULL;

    if(*head == NULL){
        *head = n;
        *tail = n;
    }else{
        
        (*tail)->next = n;
        *tail = n;    
    }
    
}

//This is how the backing of files is done, where it takes the source file,
//and then make a backup file of it, then put it's contents in it
void * backUpFile(void *param){
    //set variables up
    FILE *fp = NULL, *fp2 = NULL;
    td *myTD = (td *) param;
    int index = myTD->index;
    struct dirent* dir = myTD->d;
    char * str = malloc(PATH_MAX * sizeof(char));
    char * sourceStr = malloc(PATH_MAX * sizeof(char));
    char * readStr = malloc(BUFSIZE * sizeof(char));
    
    //malloc check
    if(str == NULL || readStr == NULL){
        printf("Memory creation issue with file name\n");
        exit(1);
    }

    //Get the name of the new file
    strncpy(str, myTD->curDir, PATH_MAX);
    strncat(str, "/", 2);
    strncat(str, BACKUPDIRNAME, 8);
    strncat(str, "/", 2);
    strncat(str, dir->d_name, FILENAMEBUF);
    char *b = ".bak";
    strncat(str, b, 4);
    if(DEBUG == 1) printf("File created: %s\n", str);

    strncpy(sourceStr, myTD->curDir, PATH_MAX);
    strncat(sourceStr, "/", 2);
    strncat(sourceStr, dir->d_name, PATH_MAX);
    
    //Open up the files, with error check
    //fp = fopen(dir->d_name, "r"); //read the source file
    fp = fopen(sourceStr, "r");
    fp2 = fopen(str, "w"); //write to dest file
    if (fp == NULL || fp2 == NULL) {
		fprintf(stderr, "Cannot open file %s or %s to read\n", sourceStr, str);
		free(str);
        free(readStr);
		exit(1);
	}
    
    //The actually coping is done here
    while(fgets(readStr, BUFSIZE, fp)){ 
        fputs(readStr, fp2);
    } 
    
    //Close the files 
    if(fclose(fp) != 0 || fclose(fp2)){
        printf("Cannot close file %s or %s\n", dir->d_name, str);
        exit(1);
    }
    
    //TODO Calculate the number of bytes copied from file to file    
    printf("[thread %x] %s\n", index, dir->d_name);
    free(str);
    free(myTD);
    free(readStr);
    return NULL;
}

DIR* getBackup(char* curDirName) {

    char* backDirName = calloc(PATH_MAX, sizeof(char));
    backDirName = strncpy(backDirName, curDirName, PATH_MAX);
    backDirName = strncat(backDirName, "/", PATH_MAX);
    backDirName = strncat(backDirName, BACKUPDIRNAME, PATH_MAX);

    // get the backup directory
    DIR* backDir = opendir(backDirName);
    //TODO Have a way for when the backup already exists
    if (backDir == NULL) {
        if (errno != ENOENT) {
            printf("%s\n", strerror(errno));
            exit(1);
        }

        // could not find directory, so make a new one
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

void iterateDir(char* curDirName) {
    if (DEBUG != 0) { printf("cur: %s\n", curDirName); }

    errno = 0;
    DIR* curDir = opendir(curDirName);

    if (curDir == NULL) {
        printf("%s\n", strerror(errno));
        exit(1);
    }

    struct dirent* dir = NULL;
    DIR* backUpDir = getBackup(curDirName);
    int fileCount = 0;
    node *head = NULL, *tail = NULL;
    char* newDirName = calloc(PATH_MAX, sizeof(char));

    while ((dir = readdir(curDir)) != NULL) {
        struct stat st;
        
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") || !strcmp(dir->d_name, BACKUPDIRNAME)) {
            continue;
        }

        memset(newDirName, '\0', sizeof(char) * PATH_MAX);
        newDirName = strncpy(newDirName, curDirName, PATH_MAX);
        newDirName = strncat(newDirName, "/", PATH_MAX);
        newDirName = strncat(newDirName, dir->d_name, PATH_MAX);

        //printf("DDNAME: %s\n", dir->d_name);
        if (stat(newDirName, &st) < 0) {
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
            // add onto the list for it to be backed up
            if (DEBUG != 0) {
                printf("File: %s\n", dir->d_name);
            }
            fileCount++;
            insert(dir, curDirName, &head, &tail);
        }


        if (S_ISDIR(st.st_mode)) {
            // it is a directory,
            // recursively call this function to read it
            if (DEBUG != 0) {
                printf("Dir: %s\n", dir->d_name);
            }

            iterateDir(newDirName);
        }
    }
    //This is where the backing of files is done
    pthread_t thr[fileCount];
    node *n = head, *temp = NULL;
    int i;

    //This can be done differently, with a global track of the threads when they are 
    //called, and have a join function for that. Just doesn't seem very useful

    //Spawn threads for all the files
    for(i = 0; i < fileCount; i++){
        //We know the thread number and the regular file
        td *myTd = malloc(sizeof(td));
        myTd->index = i;
        myTd->d = n->data;
        myTd->curDir = calloc(PATH_MAX, sizeof(char));
        myTd->curDir = strncpy(myTd->curDir, n->curDir, PATH_MAX);

        printf("[thread %x] Backing up %s\n", i, n->data->d_name);
        pthread_create(&thr[i], NULL, backUpFile, (void *)myTd);//(void *)n->data);
        
        temp = n->next;
        free(n->curDir);
        free(n);
        n = temp;
    }
    //This is where the waiting for files to be executed is done
    for(i = 0; i < fileCount; i++){
        pthread_join(thr[i], NULL);
    }
    
    //close the directories once we are done with them
    closedir(curDir);
    closedir(backUpDir);
    // reached end of directory

    free(newDirName);
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
