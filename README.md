CS 460: Project 3 - BackItUp!

4/19/23

Created by Kevin Kinney and Joseph Wiedeman

How to run:
	Run "make" (with provided Makefile) to build executables of all utilities. Running "make clean" removes the executables. Each of the executables require input as formated below:

	./BackItUp [-r]

What code does:
	Without the "-r" restore flag, the program will backup all regular files that it has permissions to in the current working directory and it's subdirectories. In each directory, a ".backup" directory will be created to contain it's backups. In order to backup a file, it must have the same or later modification time than its corresponding backup. It will backup files that do not already have a backup. When a backup is necessary, the program creats a thread for each file to copy its contents over to its backup.

	When excuted with the "-r" flag, the program will attempt to restore files in the current working directory (not the subdirectories) from their backups. In order to restore a file, the source must have earlier modification time than its corresponding backup.

Code implementation decisions:
	Since we're working with threads and recursion through directories, we thought it would be best to refer to everything as it's absolute path. Although this ment that we had to do some ugly string manipulation stuff.

	We opted to first get all the files that needed to be backed up/restored in a list before creating a thread to copy over data. This was to make the thread creation and joining easier. We also chose to use a list so that the program could handle any number of files.

	Since the "BackItUp" executable will always be in use when restoring files, its backup file will always be omitted from restoration.