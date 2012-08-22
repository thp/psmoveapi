# should be either OSC_HOST_BIG_ENDIAN or OSC_HOST_LITTLE_ENDIAN
# Apple: OSC_HOST_BIG_ENDIAN
# Win32: OSC_HOST_LITTLE_ENDIAN
# i386 LinuX: OSC_HOST_LITTLE_ENDIAN

ENDIANESS=OSC_HOST_LITTLE_ENDIAN
PLATFORM=$(shell uname)

FRAMEWORKS = -lGL -lGLU -lglut

SDL_CFLAGS  := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

TUIO_DEMO = TuioDemo
TUIO_DUMP = TuioDump
SIMPLE_SIMULATOR = SimpleSimulator
TUIO_STATIC  = libTUIO.a
TUIO_SHARED  = libTUIO.so

INCLUDES = -I./TUIO -I./oscpack
CFLAGS  = -Wall -O3 $(SDL_CFLAGS)
#CFLAGS  = -g -Wall -O3 $(SDL_CFLAGS)
CXXFLAGS = $(CFLAGS) $(INCLUDES) -D$(ENDIANESS)
SHARED_OPTIONS = -shared -Wl,-soname,$(TUIO_SHARED)

ifeq ($(PLATFORM), Darwin)
	TUIO_SHARED  = libTUIO.dylib
	FRAMEWORKS =  -framework OpenGL -framework GLUT
	SHARED_OPTIONS = -dynamiclib -Wl,-dylib_install_name,$(TUIO_SHARED)
endif

DEMO_SOURCES = TuioDemo.cpp
DEMO_OBJECTS = TuioDemo.o
DUMP_SOURCES = TuioDump.cpp
DUMP_OBJECTS = TuioDump.o
SIMULATOR_SOURCES = SimpleSimulator.cpp
SIMULATOR_OBJECTS = SimpleSimulator.o

TUIO_SOURCES = ./TUIO/TuioClient.cpp ./TUIO/TuioServer.cpp ./TUIO/TuioTime.cpp
OSC_SOURCES = ./oscpack/osc/OscTypes.cpp ./oscpack/osc/OscOutboundPacketStream.cpp ./oscpack/osc/OscReceivedElements.cpp ./oscpack/osc/OscPrintReceivedElements.cpp ./oscpack/ip/posix/NetworkingUtils.cpp ./oscpack/ip/posix/UdpSocket.cpp

COMMON_SOURCES = $(TUIO_SOURCES) $(OSC_SOURCES)
COMMON_OBJECTS = $(COMMON_SOURCES:.cpp=.o)

all: dump demo simulator static shared

static:	$(COMMON_OBJECTS)
	ar rcs $(TUIO_STATIC) $(COMMON_OBJECTS)

shared:	$(COMMON_OBJECTS)
	$(CXX) -lpthread $+ $(SHARED_OPTIONS) -o $(TUIO_SHARED)
	
dump:	$(COMMON_OBJECTS) $(DUMP_OBJECTS)
	$(CXX) -o $(TUIO_DUMP) $+ -lpthread

demo:	$(COMMON_OBJECTS) $(DEMO_OBJECTS)
	$(CXX) -o $(TUIO_DEMO) $+ $(SDL_LDFLAGS) $(FRAMEWORKS)

simulator:	$(COMMON_OBJECTS) $(SIMULATOR_OBJECTS)
	$(CXX) -o $(SIMPLE_SIMULATOR) $+ $(SDL_LDFLAGS) $(FRAMEWORKS)

clean:
	rm -rf $(TUIO_DUMP) $(TUIO_DEMO) $(SIMPLE_SIMULATOR) $(TUIO_STATIC) $(TUIO_SHARED) $(COMMON_OBJECTS) $(DUMP_OBJECTS) $(DEMO_OBJECTS) $(SIMULATOR_OBJECTS)
