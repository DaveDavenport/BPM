# Check dependencies
PKG_CONFIG=$(shell which pkg-config)
EMPTY=
CLANGXX=$(shell which clang++)

ifneq ($(CLANGXX),$(EMPTY))
$(info Using clang)
CXX=$(CLANGXX)
endif

ifeq ($(PKG_CONFIG),$(EMPTY))
$(error pkg-config not found.)
endif

PKG_CONFIG_VERSION=$(shell $(PKG_CONFIG) --silence-errors --modversion sqlite3)

ifeq ($(PKG_CONFIG_VERSION),$(EMPTY))
$(error sqlite3 not found) 
endif


QUIET=@
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:%.cc=%.o)
PROGRAM=bpm
CXXFLAGS=-g3 -Wall -std=c++0x $(shell $(PKG_CONFIG) --cflags sqlite3)
LDXXFLAGS=
LIBS=$(shell $(PKG_CONFIG) --libs sqlite3)

all: $(PROGRAM) 


%.o: %.cc | Makefile
	$(info Compiling: $^ -> $@)
	$(QUIET)$(CXX) $(CXXFLAGS) -c -o $@ $^

$(PROGRAM): $(OBJECTS)
	$(info  Linking: $@)
	$(QUIET)$(CXX) $(LDXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(PROGRAM) $(OBJECTS)

.PHONY: plot
plot: $(PROGRAM)
	gnuplot plot.gnuplot
