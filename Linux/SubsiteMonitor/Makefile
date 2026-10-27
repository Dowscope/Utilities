CXX := g++
CPPFLAGS ?= -Isrc
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wconversion
LDFLAGS ?=
LDLIBS ?=

TARGET := subsite-monitor
SOURCES := src/main.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: all clean test debug install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c -o $@ $<

debug: CXXFLAGS := -std=c++23 -O0 -g3 -Wall -Wextra -Wpedantic -Wconversion
debug: clean all

test: $(TARGET)
	sh ./tests/test_cli.sh ./$(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)

-include $(DEPS)
