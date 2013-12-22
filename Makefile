SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:%.cc=%.o)
PROGRAM=bpm
CXXFLAGS=-g3 -Wall -std=c++0x 
LDXXFLAGS=
LIBS=-lsqlite3

$(INFO $(OBJECTS))

$(PROGRAM): $(OBJECTS)
	$(CXX) $(LDXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(PROGRAM) $(OBJECTS)

