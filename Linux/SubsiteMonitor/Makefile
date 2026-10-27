CXX := g++
CPPFLAGS ?= -Isrc
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wconversion
LDFLAGS ?=
LDLIBS ?=
NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
CPPFLAGS += $(NCURSES_CFLAGS)
LDLIBS += $(NCURSES_LIBS)

TARGET := subsite-monitor
VERSION := 0.1.0-alpha.1
CPPFLAGS += -DSUBSITE_MONITOR_VERSION='"$(VERSION)"'
SOURCES := src/main.cpp src/collector.cpp src/log_parser.cpp \
	src/statistics.cpp src/report.cpp src/tui.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
PARSER_TEST := tests/test_parser
STATISTICS_TEST := tests/test_statistics
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

$(PARSER_TEST): tests/test_parser.cpp src/log_parser.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(STATISTICS_TEST): tests/test_statistics.cpp src/statistics.o src/report.o \
	src/collector.o src/log_parser.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test: $(TARGET) $(PARSER_TEST) $(STATISTICS_TEST)
	./$(PARSER_TEST)
	./$(STATISTICS_TEST)
	sh ./tests/test_cli.sh ./$(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS) $(PARSER_TEST) $(STATISTICS_TEST)

-include $(DEPS)
