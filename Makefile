CC = gcc
CFLAGS = -O2 -s -mwindows
LIBS = -luser32 -lgdi32

SOURCES = src/main.c
EXECUTABLE = server_changer.exe

all:
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SOURCES) $(LIBS)

clean:
	del $(EXECUTABLE)

.PHONY: all clean