CFLAGS=-Wall -Wextra -Wpedantic -std=c11 -O3 -g -Iinclude
LDFLAGS=-shared -fPIC -flto -fvisibility=hidden -lm
PLUGIN=libllp.so
SOURCEDIR=src
SOURCE=$(wildcard $(SOURCEDIR)/*.c)
BUILDDIR=build

all: $(BUILDDIR)/$(PLUGIN)

$(BUILDDIR)/$(PLUGIN): $(SOURCE:$(SOURCEDIR)/%.c=$(BUILDDIR)/%.o)
	$(CC) $(LDFLAGS) -o $(@) $(^)
		
$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(@) -c $(<)

$(BUILDDIR):
	mkdir -p $@

install:
	install -Dm755 $(BUILDDIR)/$(PLUGIN) $(LADSPA_DIR)/$(PLUGIN)

clean:
	rm -rf $(BUILDDIR)

.PHONY: clean install
