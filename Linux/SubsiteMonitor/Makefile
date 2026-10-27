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
VERSION := 0.1.0-alpha.4
CPPFLAGS += -DSUBSITE_MONITOR_VERSION='"$(VERSION)"'
SOURCES := src/main.cpp src/config.cpp src/nginx_config.cpp src/collector.cpp src/log_parser.cpp \
	src/statistics.cpp src/report.cpp src/tui.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
PARSER_TEST := tests/test_parser
STATISTICS_TEST := tests/test_statistics
CONFIG_TEST := tests/test_config
NGINX_CONFIG_TEST := tests/test_nginx_config
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SYSCONFDIR ?= /etc
CONFIGDIR ?= $(SYSCONFDIR)/subsite-monitor
CONFIGFILE ?= $(CONFIGDIR)/monitor.conf

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

$(CONFIG_TEST): tests/test_config.cpp src/config.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(NGINX_CONFIG_TEST): tests/test_nginx_config.cpp src/nginx_config.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test: $(TARGET) $(PARSER_TEST) $(STATISTICS_TEST) $(CONFIG_TEST) $(NGINX_CONFIG_TEST)
	./$(PARSER_TEST)
	./$(STATISTICS_TEST)
	./$(CONFIG_TEST)
	./$(NGINX_CONFIG_TEST)
	sh ./tests/test_cli.sh ./$(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"
	install -d -m700 "$(DESTDIR)$(CONFIGDIR)"
	if [ ! -e "$(DESTDIR)$(CONFIGFILE)" ]; then \
		install -m600 config/monitor.conf.example \
			"$(DESTDIR)$(CONFIGFILE)"; \
	else \
		chmod 600 "$(DESTDIR)$(CONFIGFILE)"; \
	fi

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS) $(PARSER_TEST) $(STATISTICS_TEST) \
		$(CONFIG_TEST) $(NGINX_CONFIG_TEST)

-include $(DEPS)
