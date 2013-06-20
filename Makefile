VPATH := ./src

OBJECTS := confile.o Debug.o MemoryManager.o Mint.o MrswLock.o Nonreentrant.o Statistics.o TonyLowDebug.o CSocket.o BaseLib.o Buffer.o ThreadPool.o Log.o

LIBNAME := libmy.a

CC := g++
CFLAGS := -Wno-write-strings -I./include
AR := ar
#CFLAGS += -I./include

mylib: $(OBJECTS)
	$(AR) rcs $(LIBNAME) $(OBJECTS)

%.o:%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:clean

clean:
	rm -rf *.o
