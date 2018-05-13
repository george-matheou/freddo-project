include ../../Makefile.in

FREDDO_BUILD 			= ../../../../freddo/Build/Collections
TILE_MATRIX_OBJS		= $(FREDDO_BUILD)/TileMatrix/TileMatrix.o $(FREDDO_BUILD)/TileMatrix/BMatrix.o $(FREDDO_BUILD)/Matrix/Matrix.o   
TILE_MATRIX_INC_DIR		=../$(FREDDO_INC_DIR)/freddo/Collections/TileMatrix/

CXXFLAGS = -g -Wall -std=c++11 -static -fopenmp -DDEBUG $(PLASMA_INC_DIR) -I$(TILE_MATRIX_INC_DIR)
#CXXFLAGS = -O3 -fopenmp -DDEBUG  -I$(PLASMA_INC_DIR) -I$(TMATRIX_INC_DIR)

LOBJS = CoreBlasTile.o 

LIBS = libCoreBlasTile.a

TARGET = test

$(LIBS): $(LOBJS)
	$(AR) r $(LIBS) $(LOBJS) $(TILE_MATRIX_OBJS)
	ranlib $(LIBS)

$(TARGET): CoreBlasTileTest.o $(LIBS)
	$(CXX) $(CXXFLAGS) -o $@ CoreBlasTileTest.o $(TILE_MATRIX_OBJS) $(LIBS) $(PLASMA_LIB_DIR) $(PLASMA_LIBS)
	
all: $(LIBS)

clean_all:
	rm -f *.o $(LOBJS) $(LIBS)

.cpp.o :
	$(CXX) $(CXXFLAGS) -c $<
