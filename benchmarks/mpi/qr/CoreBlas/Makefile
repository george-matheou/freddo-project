
PLASMA_ROOT = /home/geomat/plasma
PLASMA_INC_DIR = $(PLASMA_ROOT)/include
PLASMA_LIB_DIR = $(PLASMA_ROOT)/lib
PLASMA_LIBS = -lplasma -lcoreblas #-lquark -lpthread
#
FREDDO_BUILD 			= ../../../../freddo/Build/Collections
TILE_MATRIX_OBJS		= $(FREDDO_BUILD)/TileMatrix/TileMatrix.o $(FREDDO_BUILD)/TileMatrix/BMatrix.o $(FREDDO_BUILD)/Matrix/Matrix.o   
TILE_MATRIX_INC_DIR		=../../../../install_freddo/include/freddo/Collections/TileMatrix/

#
CXX = g++
CXXFLAGS = -g -Wall -std=c++11  -DDEBUG -I$(PLASMA_INC_DIR) -I$(TILE_MATRIX_INC_DIR) #-static -fopenmp
#CXXFLAGS = -O3 -fopenmp -DDEBUG -I$(BLAS_INC_DIR) -I$(PLASMA_INC_DIR) -I$(TMATRIX_INC_DIR)

LOBJS = CoreBlasTile.o 

LIBS = libCoreBlasTile.a

TARGET = test

$(LIBS):	$(LOBJS)
	$(AR) r $(LIBS) $(LOBJS) $(TILE_MATRIX_OBJS)
	ranlib $(LIBS)

$(TARGET): CoreBlasTileTest.o $(LIBS)
	$(CXX) $(CXXFLAGS) -o $@ CoreBlasTileTest.o $(TILE_MATRIX_OBJS) $(LIBS) -L$(PLASMA_LIB_DIR) $(PLASMA_LIBS) #$(BLAS_LIBS)
	
all:	$(LIBS)

clean_all:
	rm -f *.o $(LOBJS) $(LIBS)

.cpp.o :
	$(CXX) $(CXXFLAGS) -c $<
