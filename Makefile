CFLAGS=-Wall -Wextra -Wpedantic -isystem$(PWD) -std=c11
LDFLAGS=-shared -fPIC -flto -fvisibility=hidden
PLUGINS=orbit delay
MAKEFLAGS+=--no-print-directory

all: $(PLUGINS)

orbit: LDFLAGS+=-lm
$(PLUGINS):
	$(MAKE) -f plugin.mk PLUGIN=$(@) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"
		
.c.o:
	$(CC) $(CFLAGS) -o $(@) -c $(<)

clean:
	rm -rf build

.PHONY: $(PLUGINS)
