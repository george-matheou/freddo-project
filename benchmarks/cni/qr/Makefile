include ../Makefile.in

# CoreBlas libraries
COREBLAS_ROOT 	 		= ./CoreBlas
COREBLAS_INC_DIR 		= $(COREBLAS_ROOT)
COREBLAS_LIB_DIR 		= $(COREBLAS_ROOT)
COREBLAS_LIBS 	 		= -lCoreBlasTile
TILE_MATRIX_INC_DIR	 	= $(FREDDO_INC_DIR)/freddo/Collections/TileMatrix/

all: core_blas_lib bin/qr_right_looking

core_blas_lib:
	make -C $(COREBLAS_ROOT)

bin/qr_right_looking: Check_Accuracy.o qr_right_looking.cpp
	$(CXX) -Wall -std=c++11  -O3 -o  bin/qr_right_looking qr_right_looking.cpp Check_Accuracy.o \
	$(BLAS_INC_DIR) $(PLASMA_INC_DIR) -I$(COREBLAS_INC_DIR) -I$(TILE_MATRIX_INC_DIR) -I$(FREDDO_INC_DIR) \
	-L$(COREBLAS_LIB_DIR) $(COREBLAS_LIBS) \
	$(PLASMA_LIB_DIR) $(PLASMA_LIBS) \
	$(BLAS_LIB_DIR) $(BLAS_LIBS) \
	-L$(FREDDO_LIB_DIR) $(FREDDO_LIBS)

Check_Accuracy.o: Check_Accuracy.cpp
	$(CXX) -c Check_Accuracy.cpp  -std=c++11 -O3 $(LAPACK_INCLUDES) $(LAPACK_LIBS_PATHS) $(LAPACK_LIBS)

bin/qr_serial_lapack: qr_serial_lapack.cpp
	$(CXX) -Wall -std=c++11  -O3 -o bin/qr_serial_lapack qr_serial_lapack.cpp  $(LAPACK_INCLUDES) $(LAPACK_LIBS_PATHS) $(LAPACK_LIBS)

.PHONY: clean
clean:
	rm -f --preserve-root Check_Accuracy.o bin/qr_right_looking
	rm -f --preserve-root -f $(COREBLAS_ROOT)/*.o  $(COREBLAS_ROOT)/*.a

	
