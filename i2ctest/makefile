OBJs := $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
DEPs := $(OBJs:%.o=%.d)
BIN  := main

# Use a modern language
CPPFLAGS += --std=c++0x

# Turn on "all" warnings
CPPFLAGS += -Wall

# Generate .d files with dependency info
CPPFLAGS += -MD -MP

# Debuggable and optimized.
CPPFLAGS += -O2

LDFLAGS += -lwiringPi -lpthread -lm

.PHONY: all
all: subprocesses main

main:	$(OBJs)

.PHONY: clean
clean:
	@rm -fv main $(OBJs) $(DEPs) *.o

	
.PHONY: subprocesses
subprocesses:
