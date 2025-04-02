SOURCES=$(wildcard $(PLUGIN)/*.c)

build/lib$(PLUGIN).so: $(SOURCES:$(PLUGIN)/%.c=build/$(PLUGIN)/%.o) | build
	$(CC) $(LDFLAGS) -o $(@) $(^)

build/$(PLUGIN)/%.o: $(PLUGIN)/%.c | build/$(PLUGIN)
	$(CC) $(CFLAGS) -o $(@) -c $(<)

build/$(PLUGIN):
	mkdir -p build/$(PLUGIN)

build:
	mkdir -p build
