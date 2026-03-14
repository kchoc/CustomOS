$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) $(INCLUDES) -c $< -o $@