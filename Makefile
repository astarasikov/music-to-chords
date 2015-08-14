APPNAME=midiate
CC=clang++
CFLAGS=-pg $(shell pkg-config --libs --cflags sndfile)
LDFLAGS=

CFILES = \
	midiate.cpp

OBJFILES=$(patsubst %.cpp,%.o,$(CFILES))

all: $(APPNAME)
	rm out.wav

$(APPNAME): $(OBJFILES)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJFILES)

$(OBJFILES): %.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(APPNAME)
	rm *.o
