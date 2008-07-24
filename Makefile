CXXFLAGS=-g -O2

all: caissa

caissa: State.o Game.o
	$(CXX) $(LDLIBS) $(LDFLAGS) $(CXXFLAGS) State.o Game.o -o caissa

clean:
	-rm *.o
	-rm caissa
