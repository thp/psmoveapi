
TARGET = psmove_x11mouse

OBJS = x11mouse.o


UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    # Mac OS X: Include custom xdo-compatible implementation
    OBJS += xdo_osx.o
    LDFLAGS += -framework ApplicationServices -framework Foundation -framework AppKit
else
    # Linux and others: Use the real libxdo
    LDFLAGS += -lxdo
endif

CFLAGS += -I../../../include/
LDFLAGS += -lpsmoveapi -L../../../build/

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)

distclean: clean

.PHONY: all clean distclean
.DEFAULT: all

