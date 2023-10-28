CXX = gcc
CXXFLAGS = -Wall -Wno-unused-function
SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:.c=.o)
EXEC = main

all: $(EXEC)

$(EXEC): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run:
	clear
	./main script.txt

clean:
	rm -f $(EXEC) $(OBJ_FILES)
	clear

.PHONY: all clean