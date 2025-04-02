CFLAGS=-Wall -Wextra -Wpedantic -isystem$(PWD) -std=c11
LDFLAGS=-shared -fPIC -flto -fvisibility=hidden
PLUGINS=orbit

all: $(PLUGINS)

orbit: LDFLAGS+=-lm
$(PLUGINS):
	$(MAKE) -f plugin.mk \
		PLUGIN=$(@) \
		CFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)"
		
.c.o:
	$(CC) $(CFLAGS) -o $(@) -c $(<)

clean:
	rm -rf *.so **/*.o

.PHONY: $(PLUGINS)
