CFLAGS=-g -O2 -Wall -I . -I include
CXXFLAGS=-g -O2 -DUSE_WAYLAND -Wall -std=c++20 -I . -I include $(shell pkg-config --cflags glfw3 glm stb wayland-client libonnxruntime)
LDFLAGS=$(shell pkg-config --libs glfw3 glm stb wayland-client libonnxruntime)
OBJ=$(shell find -name "*.cc" | sed -e 's/\.cc$$/.o/g') $(shell find -name "*.c" | sed -e 's/\.c$$/.o/g')
TARGET=_builtin.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJ)
