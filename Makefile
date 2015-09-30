OBJS = ftserve.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -std=c++0x -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

ftserver : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o ftserve

ftserver.o : ftserve.cpp
	$(CC) $(CFLAGS) ftserve.cpp

clean:
	\rm *.o ftserve
