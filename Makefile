all: loader kernel

.PHONY : clean
clean:
	$(MAKE) -C src/loader clean
	$(MAKE) -C src/kernel clean

# Compile
loader: force_look
	$(MAKE) -j -C src/loader

kernel: force_look
	$(MAKE) -j -C src/kernel

force_look:
	@true