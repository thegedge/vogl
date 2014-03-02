CC=clang++
CCFLAGS=-c -Wall -O2 -isystem /usr/local/include/freetype2 -I. -std=c++11
LDFLAGS=-L/usr/local/lib -lglfw3 -lfreetype -framework OpenGL
SOURCES=example.cpp text_renderer.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: example

example: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o example

.cpp.o:
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -rf *.o example
