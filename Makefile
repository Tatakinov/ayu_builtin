CFLAGS=-g -Wall -I . -I include
CXXFLAGS=-g -DUSE_WAYLAND -Wall -std=c++20 -I . -I include $(shell pkg-config --cflags glfw3) $(shell pkg-config --cflags glm) $(shell pkg-config --cflags stb)
LDFLAGS=$(shell pkg-config --libs glfw3) $(shell pkg-config --libs glm) $(shell pkg-config --libs stb) $(shell pkg-config --libs wayland-client)
OBJ=$(shell find -name "*.cc" | sed -e 's/\.cc$$/.o/g') $(shell find -name "*.c" | sed -e 's/\.c$$/.o/g')
TARGET=_builtin.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJ)
