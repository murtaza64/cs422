# -*-Makefile-*-

OBJDIRS += $(KERN_OBJDIR)/futex

KERN_SRCFILES += $(KERN_DIR)/futex/futex_map.c
KERN_SRCFILES += $(KERN_DIR)/futex/futex_queue.c
KERN_SRCFILES += $(KERN_DIR)/futex/futex.c


$(KERN_OBJDIR)/fs/%.o: $(KERN_DIR)/fs/%.c
	@echo + cc[KERN/fs] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/fs/%.o: $(KERN_DIR)/fs/%.S
	@echo + as[KERN/fs] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<
