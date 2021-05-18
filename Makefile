CWARNINGS=-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2
CFLAGS=$(CWARNINGS) -Werror

a.out: src/paex_sine_c++.cpp vendors/portaudio/include/portaudio.h
	c++ $(CFLAGS) -Ivendors/portaudio/include/ -Lvendors/portaudio/lib/.libs -lportaudio src/paex_sine_c++.cpp


