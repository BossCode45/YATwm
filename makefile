CXXFLAGS := -g `pkg-config --cflags x11`
LDFLAGS := `pkg-config --libs x11`

all: YATwm

HEADERS = \
	config.h \
	structs.h
SOURCES = \
	main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

YATwm: $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f YATwm $(OBJECTS)
