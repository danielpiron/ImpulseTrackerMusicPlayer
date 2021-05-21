CWARNINGS=-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2
CFLAGS=$(CWARNINGS) -Werror -std=c++14 -fsanitize=address

a.out: src/paex_sine_c++.cpp vendors/portaudio/include/portaudio.h
	c++ $(CFLAGS) -Ivendors/portaudio/include/ -Lvendors/portaudio/lib/.libs -lportaudio src/paex_sine_c++.cpp


run_tests: tests/main.cpp
	c++ $(CFLAGS) -Ivendors/googletest/googletest/include -Lvendors/googletest/lib -lgtest tests/main.cpp -o run_tests