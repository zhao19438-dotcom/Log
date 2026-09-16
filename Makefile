all:
	$(MAKE) -C example
	$(MAKE) -C bench
	$(MAKE) -C test

test:
	$(MAKE) -C test test

clean:
	$(MAKE) -C example clean
	$(MAKE) -C bench clean
	$(MAKE) -C test clean

.PHONY: all test clean
