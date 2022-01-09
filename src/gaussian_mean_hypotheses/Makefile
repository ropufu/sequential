
PNAME = simulator.out

CC = g++-11

CFLAGS = -std=c++20 -Wall -O3 -pthread

PATHINC = -I./../../../../include -I./../../../aftermath/src

PATHLIB =

LDFLAGS = -pthread

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

