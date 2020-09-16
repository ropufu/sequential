
PNAME = tests.out

CC = g++-10

CFLAGS = -std=c++20 -Wall -O3

PATHINC = -I./../../../aftermath/src -I./../../../../include -I./..

PATHLIB =

LDFLAGS = 

.VPATH: .

$(PNAME): $(patsubst %.cpp, %.o, $(wildcard *.cpp))
	$(CC) $^ -o $@ $(PATHLIB) $(LDFLAGS) $(LDLIBS)
	rm -rf *.o *.d

%.o: %.cpp
	$(CC) $< $(CFLAGS) -c -MD $(PATHINC)

include $(wildcard *.d)

.PHONY: clean

clean:
	rm -rf *.o *.d

