############################################################
# unit test makefile
############################################################

######################################
# Set variable
######################################	
CC	= gcc
ROOTDIR = ../
INCLUDE = -I$(ROOTDIR)lib/libutil
CFLAGS = $(INCLUDE) -Wall -fPIC -g

TARGET = unit-tester
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY: $(TARGET)
all: $(TARGET)
######################################
# Compile & Link
# 	Must use \tab key after new line
######################################
$(TARGET): $(OBJ)
	libtool --mode=link $(CC) $(CFLAGS) -L/usr/lib/x86_64-linux-gnu/ $(OBJ) -o $(TARGET) \
	-static -lpicohttpparser -ltester -lutils -lpthread -lm -ldl -lutil -lconfig -luuid

######################################
# Clean 
######################################
clean: 
	rm -rf $(OBJ) $(TARGET) .libs
