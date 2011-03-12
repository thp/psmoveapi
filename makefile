
PROG=psmove-example

all: $(PROG)

OBJS=hidapi/hid.o psmove.o $(PROG).o
CFLAGS+=-Wall -g -c -std=c99 -DPSMOVE_DEBUG -I./hidapi/
LIBS=-framework IOKit -framework CoreFoundation

$(PROG): $(OBJS)
	g++ -Wall -g $^ $(LIBS) -o $(PROG)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(PROG) $(OBJS)

.PHONY: clean

