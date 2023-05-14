############################################################
# libtester makefile
############################################################

######################################
# Set variable
######################################	
CC	= gcc
INCLUDE = -I../../lib/libutil -I../../src
CFLAGS = $(INCLUDE) -Wall -fPIC -g

TARGET = example_tester
SRC = $(wildcard *.c)

OBJ = $(SRC:.c=.o)
	
all: $(TARGET)
######################################
# Compile & Link
# 	Must use \tab key after new line
######################################
$(TARGET): $(OBJ)
	libtool --mode=link $(CC) $(CFLAGS) -L/usr/lib/x86_64-linux-gnu/ $(OBJ) -o $(TARGET) \
	-lpicohttpparser -lutils -lpthread -lm -ldl -lutil -lconfig -ltester

######################################
# Clean 
######################################
clean:
	rm -rf $(OBJ) $(TARGET) .libs