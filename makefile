CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wno-unused-function
SRC_FILES = $(wildcard *.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)
EXEC = main

all: $(EXEC)

$(EXEC): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run:
	clear
	./main script.txt

clean:
	rm -f $(EXEC) $(OBJ_FILES)
	clear

.PHONY: all clean