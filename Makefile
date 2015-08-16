TARGET = qttp

CC = g++
CFLAGS = -Wall -std=c++11 -Iinclude/

LINKER = g++ -o
LFLAGS = -Wall -pthread -Iinclude/ include/http_parser.o

SRCDIR = src
OBJDIR = build
BINDIR = bin

RM = rm

SHP2HEFILES = shp2he.o 
SHP2HEOBJECTS = $(addprefix $(OBJDIR)/,$(SHP2HEFILES))

QTTPFILES = main.o qttp.o connection_handler_epoll.o connection_queue.o connection_worker.o
QTTPOBJECTS = $(addprefix $(OBJDIR)/,$(QTTPFILES))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled" $@ 

all: $(BINDIR)/qttp
shp2he: $(BINDIR)/shp2he


$(BINDIR)/qttp: $(QTTPOBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(QTTPOBJECTS)
	@echo "Linked" $@

$(BINDIR)/shp2he: $(SHP2HEOBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(SHP2HEOBJECTS)
	@echo "Linked" $@

.PHONEY: debug
debug: CFLAGS += -g
debug: all

.PHONEY: clean
clean:
	@$(RM) $(OBJDIR)/*
	@echo "Cleanup complete!"

.PHONEY: remove
remove: clean
	@$(RM) $(BINDIR)/*
	@echo "Executable removed!"
