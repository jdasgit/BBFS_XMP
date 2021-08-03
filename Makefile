COMPILER = gcc
CFLAGS = -Wall
FILESYSTEM_FILES = fusexmp.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(CFLAGS) $(FILESYSTEM_FILES) -o lsysfs `pkg-config fuse --cflags --libs`
	echo 'To Mount: ./fusexmp -f rootDir mountDir'

clean:
	rm fusexmp 
