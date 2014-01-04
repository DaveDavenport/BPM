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

PREFIX?=$(HOME)/.local/

SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:%.cc=build/objects/%.o)
PROGRAM=bpm
CXXFLAGS=-g3 -Wall -std=c++0x $(shell $(PKG_CONFIG) --cflags sqlite3)
LDXXFLAGS=
LIBS=$(shell $(PKG_CONFIG) --libs sqlite3)


#
BUILD_DIR=build
OBJECTS_DIR=$(BUILD_DIR)/objects

all: $(BUILD_DIR)/$(PROGRAM) 


$(BUILD_DIR):
	$(QUIET)mkdir -p $@ 

$(OBJECTS_DIR): $(BUILD_DIR) 
	$(QUIET)mkdir -p $@

build/objects/%.o: %.cc | Makefile $(OBJECTS_DIR) 
	$(info Compiling: $^ -> $@)
	$(QUIET)$(CXX) $(CXXFLAGS) -c -o $@ $^

build/$(PROGRAM): $(OBJECTS)
	$(info  Linking: $@)
	$(QUIET)$(CXX) $(LDXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf build 

.PHONY: plot
plot: $(BUILD_DIR)/$(PROGRAM)
	gnuplot plot.gnuplot


.PHONY: doc
doc:
	doxygen	doc/doxygen.doxy


install: $(BUILD_DIR)/$(PROGRAM)
	install $^ $(PREFIX)/bin/$(PROGRAM)
	install doc/bpm.1 $(PREFIX)/share/man/man1/
