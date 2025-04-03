CFLAGS=-Wall -Wextra -Wpedantic -isystem$(PWD) -std=c11 -O3 -g
LDFLAGS=-shared -fPIC -flto -fvisibility=hidden
PLUGINS=orbit delay orbital_delay
MAKEFLAGS+=--no-print-directory

all: $(PLUGINS)

orbit: LDFLAGS+=-lm
orbital_delay: LDFLAGS+=-lm
$(PLUGINS):
	$(MAKE) -f plugin.mk PLUGIN=$(@) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"
		
.c.o:
	$(CC) $(CFLAGS) -o $(@) -c $(<)

install:
	mkdir -p $(LADSPA_DIR)
	install -Dm755 $(PLUGINS:%=build/lib%.so) $(LADSPA_DIR)

clean:
	rm -rf build

.PHONY: $(PLUGINS)
