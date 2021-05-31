CWARNINGS=-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2
CFLAGS=$(CWARNINGS) -g -Werror -std=c++14 -fsanitize=address

a.out: src/paex_sine_c++.cpp vendors/portaudio/include/portaudio.h src/Channel.h src/Sample.h
	c++ $(CFLAGS) -Ivendors/portaudio/include/ -Lvendors/portaudio/lib/.libs -lportaudio src/paex_sine_c++.cpp


run_tests: tests/main.cpp tests/test_Sample.cpp tests/test_Channel.cpp tests/test_Mixer.cpp src/Channel.h src/Sample.h
	c++ $(CFLAGS) -Ivendors/googletest/googletest/include -Isrc -Lvendors/googletest/lib -lgtest tests/main.cpp tests/test_Sample.cpp tests/test_Channel.cpp tests/test_Mixer.cpp -o run_tests