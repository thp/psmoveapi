
TARGET = psmove_x11mouse

OBJS = x11mouse.o

CFLAGS += -I../../../include/
LDFLAGS += -lxdo -lpsmoveapi -L../../../build/

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)

distclean: clean

.PHONY: all clean distclean
.DEFAULT: all

