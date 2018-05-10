all: memdbg.so memdbgtest

memdbg.so: memdbg_so.c Makefile
	gcc -Wall -shared -fPIC -o $@ $< -ldl

memdbgtest: test.cpp Makefile
	g++ -Wall -o $@ $<

clean:
	rm -f memdbg.so memdbgtest

.PHONY: all clean
