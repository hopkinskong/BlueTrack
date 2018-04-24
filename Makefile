TARGET = bluetrack-backend
LIBS = -lpthread -lzmq -ljansson -lsqlite3 -lstdc++ -lopencv_core -lopencv_imgproc -lopencv_highgui
CC = gcc
CFLAGS = -g -Wall -O3

.PHONY: clean all default

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c)) $(patsubst %.c, %.o, $(wildcard */*.c))
OBJECTS += $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f b64/*.o
	-rm -f $(TARGET)
