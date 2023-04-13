.SUFFIXES: .c .o
# _DEFAULT_SOURCE is for DT_DIR
CCFLAGS = -std=c99 -D_DEFAULT_SOURCE -pedantic -Wall -Werror -pthread
OPTIONS = -g

build:
	gcc ${CCFLAGS} ${OPTIONS} -o exec BackItUp.c

clean:
	rm -f exec

# to run valgrind/check for memory problems run the following:
# valgrind --track-origins=yes --leak-check=full ./"my-file-name" "args..."
