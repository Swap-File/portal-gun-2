#Gstreamer stuff
LIBS=-L/usr/local/lib -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0
CPPFLAGS=-pthread -I/usr/local/include/gstreamer-1.0 -I/usr/local/lib/gstreamer-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include

# Use a modern language
CPPFLAGS += --std=c++0x

# Turn on "all" warnings
CPPFLAGS += -Wall -lwiringPi 
 
CPPFLAGS += -O2 
	
all:
	gcc $(LIBS) $(CPPFLAGS) *.cpp -o popen
