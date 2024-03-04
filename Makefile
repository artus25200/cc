OBJS	= *.o
OBJS    := $(filter-out output.o, $(OBJS))
SOURCE	= *.c
HEADER	= *.h
OUT	= cc
CC	 = gcc
FLAGS	 = -g -c -Wall -ggdb
LFLAGS	 =

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)
	$(MAKE) clean


*.o: *.c
	$(CC) $(FLAGS) *.c


clean:
	rm -f $(OBJS)

asm:
	nasm -f macho64 output.asm
	ld output.o -lSystem
