all:
	$(MAKE) -C example
	$(MAKE) -C bench

clean:
	$(MAKE) -C example clean
	$(MAKE) -C bench clean

.PHONY: all clean
