BIULD_DIR         := build

all: loader kernel

.PHONY: clean
clean:
	$(MAKE) -C src/loader clean
	$(MAKE) -C src/kernel clean

# Compile
loader: force_look | $(BIULD_DIR)
	$(MAKE) -j -C src/loader

kernel: force_look | $(BIULD_DIR)
	$(MAKE) -j -C src/kernel

$(BIULD_DIR):
	mkdir $(BIULD_DIR)

force_look:
	@true