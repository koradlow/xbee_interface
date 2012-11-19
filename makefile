#Define the compiler we want to use
CC = g++
#Define the compiler options for this project
CFLAGS += -Wall -O0 -g -std=gnu++0x
#Define the libraries that are used for this project
LDLIBS += -lgbee

#Define the output target
TARGET = test

#All source packages
SOURCES = ./test_app.cpp ./xbee_if.cpp
VPATH :=

#Define all object files
#(remove path information from source files)
COMMON_OBJS := $(patsubst %.cpp, %.o, $(notdir $(SOURCES)))

#Build all object files
%.o : %.cpp $(SOURCES)
	@echo creating "$@" ...
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(COMMON_OBJS)
	@echo building target binary "$(TARGET)" ...
	$(CC) -o $(TARGET) $(COMMON_OBJS) $(LDLIBS)

all: $(TARGET)

clean:
	rm -f $(COMMON_OBJS)

PREFIX:= /usr/local

install: all
	test -d $(PREFIX) || mkdir $(PREFIX)
	test -d $(PREFIX)/bin || mkdir $(PREFIX)/bin
	install -m 0755 $(TARGET) $(PREFIX)/bin; \

uninstall:
	test $(PREFIX)/bin/$(TARGET)
	rm -f $(PREFIX)/bin/$(TARGET)
