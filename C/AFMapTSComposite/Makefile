# Configuration
SRC_DIR = .
BIN ?= ./bin
XML2INC ?= /usr/include/libxml2/libxml
ESPAINC ?=
GSL_SCI_INC ?= /usr/include/gsl
GSL_SCI_LIB ?= /usr/local/lib
GDAL_INC ?= /usr/include/gdal
GDAL_LIB ?= /usr/lib

# Set up compile options
CC = gcc
RM = rm -f
MV = mv
EXTRA = -Wall -Wextra -g
FFLAGS=-g -fdefault-real-8

# Define the include files
INC = $(wildcard $(SRC_DIR)/*.h)
INCDIR  = -I. -I$(SRC_DIR) -I$(GSL_SCI_INC) -I$(XML2INC) -I$(ESPAINC) -I$(GSL_SCI_INC)
NCFLAGS = $(EXTRA) $(INCDIR)

# Define the source code and object files
SRC = $(wildcard $(SRC_DIR)/*.c) 
OBJ = $(SRC:.c=.o)

# Define the object libraries
LIB = -L$(GSL_SCI_LIB) -L$(GDAL_LIB) -lz -lpthread -lrt -lgsl -lgslcblas -lm -lgdal
# Define the executable
EXE = composite

# Target for the executable
all: $(EXE)

composite: $(OBJ) $(INC)
	$(CC) $(NCFLAGS) -o composite $(OBJ) $(LIB)

clean: 
	$(RM) $(BIN)/$(EXE)
	$(RM) $(BIN)/variables
	$(RM) *.o


$(BIN):
	mkdir -p $(BIN)

install: $(BIN)
	mv $(EXE) $(BIN)
	cp variables $(BIN)


$(OBJ): $(INC)

.c.o:
	$(CC) $(NCFLAGS) $(INCDIR) -c $<

