OBJs := $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
DEPs := $(OBJs:%.o=%.d)
BIN  := portal

# Use a modern language
CPPFLAGS += --std=c++0x

# Turn on "all" warnings
CPPFLAGS += -Wall

# Fix http://gcc.gnu.org/bugzilla/show_bug.cgi?id=42748
#CPPFLAGS += -Wno-psabi

# Generate .d files with dependency info
CPPFLAGS += -MD -MP

# Debuggable and optimized.
#CPPFLAGS += -g -O2 
CPPFLAGS += -O2 

LDFLAGS += -lwiringPi -lpthread -lm

# Tell the cpp file where assets are stored.
CPPFLAGS += -DASSET_DIR="\"$(assetdir)\""

# libpng for reading in textures
LDFLAGS += $(shell libpng-config --libs)

.PHONY: all
all: $(BIN)

$(BIN): $(OBJs)

.PHONY: clean
clean:
	@rm -fv $(BIN) $(OBJs) $(DEPs) *.o

-include $(DEPs)

