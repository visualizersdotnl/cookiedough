
all:

#	Optimized.
#	g++ -o bison test.cpp FM_BISON.cpp sinus-LUT.cpp synth-error.cpp synth-LFOs.cpp synth-midi.cpp synth-MOOG-ladder.cpp -std=c++14 -O3 -fno-exceptions

#	Debug (Valgrind).
	g++ -o bison test.cpp FM_BISON.cpp random.cpp sinus-LUT.cpp synth-error.cpp synth-cosine-tilt.cpp synth-midi.cpp synth-MOOG-ladder.cpp -std=c++11 -O0 -g
#	valgrind --leak-check=yes -v ./bison