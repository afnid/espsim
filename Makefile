# I don't bother creating archives for small projects, you want to make it a lib, go ahead.
# Should create a good regression test example.

OPEN_SDK	?= $(HOME)/dev/esp-open-sdk
SDK			?= $(OPEN_SDK)/esp_iot_sdk_v1.3.0
PORT		?= /dev/ttyUSB0
BAUD		?= 115200

SRCDIRS		= user sim
INCDIRS		= include sim
LIBS		=

INCS		:= $(foreach sdir,$(INCDIRS),$(wildcard $(sdir)/*.h))
SRCS		:= $(foreach sdir,$(SRCDIRS),$(wildcard $(sdir)/*.cpp))
OBJS		:= $(patsubst %.cpp,%.o,$(SRCS))
INCDIRS		:= $(addprefix -I,$(INCDIRS))
LIBS		:= $(addprefix -l,$(LIBS))

CFLAGS		=  $(INCDIRS) -I$(SDK)/include -DLINUX -std=c++11 -m32 -g -O2
CFLAGS		+= -Wall
CFLAGS		+= -Wpedantic
CFLAGS		+= -Wno-parentheses
CFLAGS		+= -Wdouble-promotion

all: espsim

espsim: $(SRCS) $(INCS) Makefile
	@rm -f espsim
	g++ $(CFLAGS) $(SRCS) -L /usr/lib/i386-linux-gnu -o $@

clean:
	rm -f espsim

putty:
	putty -P $(PORT) -load esp

term: burn
	gtkterm -s $(BAUD) -p $(PORT)
