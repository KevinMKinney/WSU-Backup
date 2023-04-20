CS 460: Project 3 - BackItUp!

4/19/23

Created by Kevin Kinney and Joseph Wiedeman

How to run:
	Run "make" (with provided Makefile) to build executables of all utilities. Running "make clean" removes the executables. Each of the executables require input as formated below:

	./BackItUp [-r]

What code does:
	
Without the "-r" restore flag, the program will backup all regular files that it has permissions to in the current working directory and it's subdirectories. In each directory, a ".backup" directory will be created to contain it's backups. In order to backup a file, it must have the same or later modification time than its corresponding backup. It will backup files that do not already have a backup. When a backup is necessary, the program creats a thread for each file to copy its contents over to its backup.

When excuted with the "-r" flag, the program will attempt to restore files in the current working directory and the subdirectories from their backups. In order to restore a file, the source must have earlier modification time than its corresponding backup. This will also create an empty backup if one does not exist, then look through the subdirectories to see if they have any backup files.

Code implementation decisions:

Since we're working with threads and recursion through directories, we thought it would be best to refer to everything as it's absolute path. Although this meant that we had to do some ugly string manipulation stuff.

We opted to first backup/restore all the files that needed to be backed up/restored via a thread, and put the thread number in a list after creating a thread to copy over data. This makes the thread creation and joining easier. We also chose to use a list so that the program could handle any number of threads to join later on.

We have a threadCount global variable to store all the threads that are being used or potential files that were looked at. Next there is a fileCount global variable that counts how many files were copied/overwritten during the program's run. Lastly, there is a fileSize golbal variable, which reads the file sizes from st.st_size. These are used in the main thread, so that no race conditions can occur from using them. fileSize should be equal the readings of the copying of the files bytesRead, since they are the same thing in the end. If they aren't, then it was a good size something was wrong.

We have restore function (restoreDir), and a backup function (iterateDir) for restoring and copying the files. Restore iterates through the backup directory, then after doing so, goes to another backup directory in a subdirectory. The backup function goes through the current working directory, looks through its file and recursively does the backup function again, waiting for that directory to be finished before moving onto others in the directory before it.

Since the "BackItUp" executable will always be in use when restoring files, its backup file will always be omitted from restoration.
