CWARNINGS=-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2
CFLAGS=$(CWARNINGS) -g -Werror -std=c++14 -fsanitize=address

a.out: src/paex_sine_c++.cpp vendors/portaudio/include/portaudio.h src/Channel.h src/Sample.h src/Mixer.h src/PatternEntry.h src/PatternEntry.cpp
	c++ $(CFLAGS) -Ivendors/portaudio/include/ -Isrc -Lvendors/portaudio/lib/.libs -lportaudio src/paex_sine_c++.cpp src/PatternEntry.cpp

run_tests: tests/main.cpp tests/test_Sample.cpp tests/test_Channel.cpp tests/test_Mixer.cpp tests/test_PatternData.cpp src/Channel.h src/Sample.h src/PatternEntry.h src/PatternEntry.cpp
	c++ $(CFLAGS) -Ivendors/googletest/googletest/include -Isrc -Lvendors/googletest/lib -lgtest tests/main.cpp tests/test_Sample.cpp tests/test_Channel.cpp tests/test_Mixer.cpp tests/test_PatternData.cpp src/PatternEntry.cpp -o run_tests