
EXAMPLES_SRC := $(wildcard *.cs)

EXAMPLES := $(patsubst %.cs,%.exe,$(EXAMPLES_SRC))

BINDINGS_SRC := $(wildcard ../../build/*.cs)

CSC ?= mcs

all: $(EXAMPLES)

%.exe: %.cs $(BINDINGS_SRC)
	$(CSC) -out:$@ $^

clean:
	rm -f $(EXAMPLES)

.PHONY: all clean
.DEFAULT: all

