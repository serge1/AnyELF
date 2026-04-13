CXX = g++
CXXFLAGS = -std=c++17 -Wall `pkg-config --cflags gtk+-2.0` -I../elfio -I../ELFIO
LDFLAGS = `pkg-config --libs gtk+-2.0`
ELFIO_DIR = ../elfio

SRCS = anyelf.cpp anyelfdump.cpp
OBJS = $(addprefix build/, $(SRCS:.cpp=.o))
TARGET = build/anyelf_gtk
PLUGIN_NAME = build/anyelf.wlx

all: build $(TARGET) $(PLUGIN_NAME)

build:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(PLUGIN_NAME): $(OBJS)
	$(CXX) -shared -g -fPIC -o $@ $^ $(LDFLAGS)

build/%.o: %.cpp | build
	$(CXX) $(CXXFLAGS) -g -fPIC -I$(ELFIO_DIR) -I. -c $< -o $@

clean:
	rm -rf build