CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
LDFLAGS ?= -lws2_32

TARGET = render_service.exe
SRCS = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
