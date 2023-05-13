############################################################
# tester makefile
############################################################

######################################
# Set variable
######################################	
CC	= gcc
INCLUDE = -Ilib/libutil -Ilib/picohttpparser
CFLAGS = $(INCLUDE) -Wall -fPIC -g #-fsanitize=address

SRC = $(wildcard src/*.c)
SHARED = bin/libtester.so
STATIC = bin/libtester.a
OBJ = $(SRC:.c=.o)

EXAMPLEDIR = example/tester
TESTDIR = unit_test
TESTBIN = unit-tester

.PHONY: build-libs
all: build-libs
######################################
# Compile & Link
# 	Must use \tab key after new line
######################################
build-libs: $(OBJ)
	gcc -shared -o $(SHARED) $(OBJ) -lutils -lpicohttpparser -lpthread -lm -ldl -lutil -lconfig
	ar -rcs $(STATIC) $(OBJ)

install:
	mkdir -p /var/log/shell-tester
	mkdir -p /etc/shell-tester
	cp config.cfg /etc/shell-tester/config.cfg
	cp bin/libtester.so /usr/local/lib/libtester.so
	cp bin/libtester.a /usr/local/lib/libtester.a

example:
	${MAKE} -C $(EXAMPLEDIR)

test:
	${MAKE} -C $(TESTDIR)
	./$(TESTDIR)/$(TESTBIN)

######################################
# Clean 
######################################
clean: 
	for dir in $(SUBDIR); do \
    	$(MAKE) -C $$dir -f Makefile $@; \
  	done
	rm -rf $(OBJ) $(TARGET) $(SHARED) $(STATIC)
	$(MAKE) -C $(TESTDIR) -f Makefile $@
	$(MAKE) -C $(EXAMPLEDIR) -f Makefile $@

uninstall:
	rm -rf /var/log/shell-tester
	rm -rf /etc/shell-tester
	rm -f /usr/local/lib/libtester.so
	rm -f /usr/local/lib/libtester.a