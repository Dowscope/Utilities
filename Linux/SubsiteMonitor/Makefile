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
VERSION := 0.1.0-alpha.13
CPPFLAGS += -DSUBSITE_MONITOR_VERSION='"$(VERSION)"'
DATADIR ?= /usr/share/subsite-monitor
CPPFLAGS += -DSUBSITE_MONITOR_INSTALLER_PATH='"$(DATADIR)/installer.sh"'
SOURCES := src/main.cpp src/config.cpp src/nginx_config.cpp src/collector.cpp src/log_parser.cpp \
	src/statistics.cpp src/storage.cpp src/service.cpp src/report.cpp src/updater.cpp src/tui.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
PARSER_TEST := tests/test_parser
STATISTICS_TEST := tests/test_statistics
CONFIG_TEST := tests/test_config
NGINX_CONFIG_TEST := tests/test_nginx_config
STORAGE_TEST := tests/test_storage
UPDATER_TEST := tests/test_updater
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
CPPFLAGS += -DSUBSITE_MONITOR_EXECUTABLE_PATH='"$(BINDIR)/subsite-monitor"'
SYSCONFDIR ?= /etc
CONFIGDIR ?= $(SYSCONFDIR)/subsite-monitor
CONFIGFILE ?= $(CONFIGDIR)/monitor.conf
SYSTEMD_UNITDIR ?= /usr/lib/systemd/system
STATEDIR ?= /var/lib/subsite-monitor
NGINX_CONFDIR ?= /etc/nginx/conf.d
LOGROTATEDIR ?= /etc/logrotate.d

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

$(STORAGE_TEST): tests/test_storage.cpp src/storage.o src/statistics.o \
	src/report.o src/collector.o src/log_parser.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(UPDATER_TEST): tests/test_updater.cpp src/updater.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test: $(TARGET) $(PARSER_TEST) $(STATISTICS_TEST) $(CONFIG_TEST) \
	$(NGINX_CONFIG_TEST) $(STORAGE_TEST) $(UPDATER_TEST)
	./$(PARSER_TEST)
	./$(STATISTICS_TEST)
	./$(CONFIG_TEST)
	./$(NGINX_CONFIG_TEST)
	./$(STORAGE_TEST)
	./$(UPDATER_TEST)
	sh ./tests/test_cli.sh ./$(TARGET)
	sh ./tests/test_service.sh ./$(TARGET)
	sh ./tests/test_installer_template.sh ./$(TARGET)
	python3 ./tests/test_ui_restart.py

install: $(TARGET)
	install -Dm755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"
	install -Dm644 systemd/subsite-monitor.service \
		"$(DESTDIR)$(SYSTEMD_UNITDIR)/subsite-monitor.service"
	install -d -m700 "$(DESTDIR)$(STATEDIR)"
	install -Dm755 installer.sh "$(DESTDIR)$(DATADIR)/installer.sh"
	install -d -m700 "$(DESTDIR)$(CONFIGDIR)"
	set -e; if [ ! -e "$(DESTDIR)$(CONFIGFILE)" ]; then \
		install -m600 config/monitor.conf.example \
			"$(DESTDIR)$(CONFIGFILE)"; \
	elif grep -Fqx 'access_log = /var/log/nginx/subsite-monitor-access.log' \
		"$(DESTDIR)$(CONFIGFILE)"; then \
		migrated_config="$(DESTDIR)$(CONFIGFILE).tmp.$$$$"; \
		sed 's|^access_log = /var/log/nginx/subsite-monitor-access.log$$|access_log = /var/log/subsite-monitor/access.log|' \
			"$(DESTDIR)$(CONFIGFILE)" >"$$migrated_config"; \
		chmod 600 "$$migrated_config"; \
		mv -f "$$migrated_config" "$(DESTDIR)$(CONFIGFILE)"; \
	else \
		chmod 600 "$(DESTDIR)$(CONFIGFILE)"; \
	fi
	install -d -m750 "$(DESTDIR)/var/log/subsite-monitor"
	install -d -m755 "$(DESTDIR)$(NGINX_CONFDIR)" \
		"$(DESTDIR)$(LOGROTATEDIR)"
	if [ ! -e "$(DESTDIR)$(NGINX_CONFDIR)/subsite-monitor.conf" ]; then \
		install -m644 config/nginx-log-format.conf.example \
			"$(DESTDIR)$(NGINX_CONFDIR)/subsite-monitor.conf"; \
	fi
	if [ ! -e "$(DESTDIR)$(LOGROTATEDIR)/subsite-monitor" ]; then \
		install -m644 config/subsite-monitor.logrotate \
			"$(DESTDIR)$(LOGROTATEDIR)/subsite-monitor"; \
	fi

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS) $(PARSER_TEST) $(STATISTICS_TEST) \
		$(CONFIG_TEST) $(NGINX_CONFIG_TEST) $(STORAGE_TEST) $(UPDATER_TEST)

-include $(DEPS)
