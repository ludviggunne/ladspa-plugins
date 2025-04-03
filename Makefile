CFLAGS=-Wall -Wextra -Wpedantic -std=c11 -O3 -g
LDFLAGS=-shared -fPIC -flto -fvisibility=hidden -lm
PLUGIN=libllp.so
SOURCE=$(wildcard *.c)

all: $(PLUGIN)

$(PLUGIN): $(SOURCE:%.c=%.o)
	$(CC) $(LDFLAGS) -o $(@) $(^)
		
.c.o:
	$(CC) $(CFLAGS) -o $(@) -c $(<)

install:
	mkdir -p $(LADSPA_DIR)
	install -Dm755 $(PLUGIN) $(LADSPA_DIR)/$(PLUGIN)

clean:
	rm -rf *.o $(PLUGIN)

.PHONY: clean install
