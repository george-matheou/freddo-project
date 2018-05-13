TILE_MATRIX_OBJS		=TileMatrix/TileMatrix.o TileMatrix/BMatrix.o Matrix/Matrix.o
TILE_MATRIX_INC_DIR		=TileMatrix/

CXXFLAGS = -Wall -std=c++11 -O3 $(PLASMA_INC_DIR) -I$(TILE_MATRIX_INC_DIR)

LOBJS = CoreBlasTile.o 
LIBS = libCoreBlasTile.a

TARGET = test

$(LIBS): $(LOBJS) $(TILE_MATRIX_OBJS)
	$(AR) r $(LIBS) $(LOBJS) $(TILE_MATRIX_OBJS)
	ranlib $(LIBS)

$(TARGET): $(TILE_MATRIX_OBJS) $(LIBS)
	$(CXX) $(CXXFLAGS) -o $@ CoreBlasTileTest.o $(TILE_MATRIX_OBJS) $(LIBS) $(PLASMA_LIB_DIR) $(PLASMA_LIBS)

all: $(TILE_MATRIX_OBJS) $(LIBS)

clean_all:
	rm -f *.o $(LOBJS) $(LIBS) $(TILE_MATRIX_OBJS)

TileMatrix/TileMatrix.o: TileMatrix/TileMatrix.cpp
	$(CXX) $(CXXFLAGS) -c TileMatrix/TileMatrix.cpp -o TileMatrix/TileMatrix.o

TileMatrix/BMatrix.o: TileMatrix/BMatrix.cpp
	$(CXX) $(CXXFLAGS) -c TileMatrix/BMatrix.cpp -o TileMatrix/BMatrix.o

Matrix/Matrix.o:  Matrix/Matrix.cpp
	$(CXX) $(CXXFLAGS) -c Matrix/Matrix.cpp -o Matrix/Matrix.o

.cpp.o :
	$(CXX) $(CXXFLAGS) -c $<
