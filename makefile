CXX = gcc
SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:.c=.o)
EXEC = main

all: $(EXEC)

$(EXEC): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(EXEC) $(OBJ_FILES)
	clear

run:
	clear
	./main


.PHONY: all clean
