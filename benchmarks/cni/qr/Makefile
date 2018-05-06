

FREDDO_LIB_DIR		=../../../install_freddo/lib
FREDDO_INC_DIR		=../../../install_freddo/include

## LAPACK paths and libraries
LAPACK_DIR		= /home/geomat/scalapack/lapack-3.6.1
LIBS_PATHS		= -L$(LAPACK_DIR) -L$(LAPACK_DIR)/lib -L$(FREDDO_LIB_DIR)
LIBS			= -llapacke  -llapack -lcblas -lblas -lgfortran -lm -lfreddo -pthread
INCLUDES		= -I$(LAPACK_DIR)/CBLAS/include -I$(LAPACK_DIR)/LAPACKE/include -I$(FREDDO_INC_DIR)
## Plasma Libraries
PLASMA_ROOT 	= /home/geomat/plasma
PLASMA_INC_DIR 	= $(PLASMA_ROOT)/include
PLASMA_LIB_DIR 	= $(PLASMA_ROOT)/lib
PLASMA_LIBS 	= -lplasma -lcoreblas -lquark
# CoreBlas libraries
COREBLAS_ROOT 	 = ./CoreBlas
COREBLAS_INC_DIR = $(COREBLAS_ROOT)
COREBLAS_LIB_DIR = $(COREBLAS_ROOT)
COREBLAS_LIBS 	 = -lCoreBlasTile
TILE_MATRIX_INC_DIR	 = $(FREDDO_INC_DIR)/freddo/Collections/TileMatrix/
# CBLAS libraries
BLAS_ROOT 	 =  $(LAPACK_DIR)
BLAS_INC_DIR = -I$(LAPACK_DIR)/CBLAS/include -I$(LAPACK_DIR)/LAPACKE/include
BLAS_LIB_DIR = -L$(LAPACK_DIR) -L$(LAPACK_DIR)/lib 
BLAS_LIBS    = -llapacke  -llapack -lcblas -lblas -lgfortran -lm 
# FREDDO libraries
FREDDO_LIBS 	= -lfreddo -pthread

all: core_blas_lib bin/qr_right_looking

core_blas_lib:
	make -C $(COREBLAS_ROOT)

bin/qr_right_looking: Check_Accuracy.o qr_right_looking.cpp
	g++ -Wall -std=c++11  -O3 -o  bin/qr_right_looking qr_right_looking.cpp Check_Accuracy.o \
	$(BLAS_INC_DIR) -I$(PLASMA_INC_DIR) -I$(COREBLAS_INC_DIR) -I$(TILE_MATRIX_INC_DIR) -I$(FREDDO_INC_DIR) \
	-L$(COREBLAS_LIB_DIR) $(COREBLAS_LIBS) \
	-L$(PLASMA_LIB_DIR) $(PLASMA_LIBS) \
	$(BLAS_LIB_DIR) $(BLAS_LIBS) \
	-L$(FREDDO_LIB_DIR) $(FREDDO_LIBS)

Check_Accuracy.o: Check_Accuracy.cpp
	g++ -c Check_Accuracy.cpp  -std=c++11 -O3 $(INCLUDES) $(LIBS_PATHS) $(LIBS)

bin/qr_serial_lapack: qr_serial_lapack.cpp
	g++ -Wall -std=c++11  -O3 -o bin/qr_serial_lapack qr_serial_lapack.cpp  $(INCLUDES) $(LIBS_PATHS) $(LIBS)

.PHONY: clean
clean:
	rm -f --preserve-root Check_Accuracy.o bin/qr_right_looking
	rm -f --preserve-root -f $(COREBLAS_ROOT)/*.o  $(COREBLAS_ROOT)/*.a

	
