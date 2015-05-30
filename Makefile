# ------------------------------------------------
# Generic Makefile
#
# Author: yanick.rochon@gmail.com
# Date  : 2011-08-10
#
# Changelog :
#   2010-11-05 - first version
#   2011-08-10 - added structure : sources, objects, binaries
#                thanks to http://stackoverflow.com/users/128940/beta
#
# Copied from Stackoverflow post: 
# http://stackoverflow.com/questions/7004702
# ------------------------------------------------

TARGET = qttp

CC = gcc
CFLAGS = -Wall -std=c++11

LINKER = gcc -o
LFLAGS = -Wall -pthread

SRCDIR = src
OBJDIR = build
BINDIR = bin

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
#INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

RM = rm

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)
	@echo "Linked!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(RM) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONEY: remove
remove: clean
	@$(RM) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"
