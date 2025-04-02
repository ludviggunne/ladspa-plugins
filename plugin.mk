SOURCES=$(wildcard $(PLUGIN)/*.c)

lib$(PLUGIN).so: $(SOURCES:%.c=%.o)
	$(CC) $(LDFLAGS) -o $(@) $(^)

.c.o:
	$(CC) $(CFLAGS) -o $(@) -c $(<)
