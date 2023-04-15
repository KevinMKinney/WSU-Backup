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
#define DEBUG 0
#define FILENAMEBUF 100
#define BUFSIZE 256

typedef struct ListNode{
    struct dirent* data;
    struct ListNode *next;
}node;

typedef struct thread_data{
    struct dirent* d;
    int index;
} td;

void insert(struct dirent *d, node **head, node **tail){
    node *n = (node *) malloc(sizeof(node));
    if(n == NULL) exit(1);
    n->data = d;
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
    char * str = malloc((FILENAMEBUF + 12) * sizeof(char));
    char * readStr = malloc(BUFSIZE * sizeof(char));
    
    //malloc check
    if(str == NULL || readStr == NULL){
        printf("Memory creation issue with file name\n");
        exit(1);
    }

    //Get the name of the new file
    strncpy(str, BACKUPDIRNAME, 9);
    strncat(str, dir->d_name, FILENAMEBUF);
    char *b = ".bak";
    strncat(str, b, 4);
    if(DEBUG == 1) printf("File created: %s\n", str);
    
    //Open up the files, with error check
    fp = fopen(dir->d_name, "r"); //read the source file
    fp2 = fopen(str, "w"); //write to dest file
    if (fp == NULL || fp2 == NULL) {
		fprintf(stderr, "Cannot open file %s or %s to read\n", dir->d_name, str);
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

DIR* getBackup() {
    // get the backup directory
    DIR* backDir = opendir(BACKUPDIRNAME);
    //TODO Have a way for when the backup already exists
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
    DIR* curDir = opendir(curDirName);
    struct dirent* dir = NULL;
    DIR* backUpDir = getBackup();
    int fileCount = 0;
    node *head = NULL, *tail = NULL;
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
            // add onto the list for it to be backed up
            if (DEBUG != 0) {
                printf("File: %s\n", dir->d_name);
            }
            fileCount++;
            insert(dir, &head, &tail);
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
            //TODO Subdirectory stuff
            //iterateDir(dir->d_name);
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

        printf("[thread %x] Backing up %s\n", i, n->data->d_name);
        pthread_create(&thr[i], NULL, backUpFile, (void *)myTd);//(void *)n->data);
        
        temp = n->next;
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
