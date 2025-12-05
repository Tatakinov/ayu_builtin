CXXFLAGS=-g -O2 -DUSE_WAYLAND -Wall -std=c++20 -I . -I include $(shell pkg-config --cflags sdl3 stb)
LDFLAGS=-L . $(shell pkg-config --libs sdl3 stb)
OBJ=$(shell find -name "*.cc" | sed -e 's/\.cc$$/.o/g')
TARGET=_builtin.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJ)
