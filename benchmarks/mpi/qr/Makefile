## CoreBlas libraries
COREBLAS_ROOT 	 		= ./CoreBlas
COREBLAS_INC_DIR 		= $(COREBLAS_ROOT)
COREBLAS_LIB_DIR 		= $(COREBLAS_ROOT)
COREBLAS_LIBS 	 		= -lCoreBlasTile

## CBLAS paths and libraries
BLAS_ROOT 			=$(LAPACK_DIR)
BLAS_INC_DIR 		=-I$(LAPACK_DIR)/CBLAS/include -I$(LAPACK_DIR)/LAPACKE/include
BLAS_LIB_DIR 		=-L$(LAPACK_DIR) -L$(LAPACK_DIR)/lib
BLAS_LIBS    		=-llapacke  -llapack -lcblas -lblas -lgfortran -lm

EXECS=core_blas_lib bin/qr_right_looking 

# Set the default goal of this makefile
.DEFAULT_GOAL := all

.PHONY: all
all:$(EXECS)

core_blas_lib:
	make -C $(COREBLAS_ROOT)

bin/qr_right_looking: qr_right_looking.cpp
	$(CXX_MPI) $(CXXFLAGS) -o  bin/qr_right_looking qr_right_looking.cpp  \
	$(BLAS_INC_DIR) $(PLASMA_INC_DIR) -I$(COREBLAS_INC_DIR)  -I$(FREDDO_INC_DIR) \
	-L$(COREBLAS_LIB_DIR) $(COREBLAS_LIBS) \
	$(PLASMA_LIB_DIR) $(PLASMA_LIBS) \
	$(BLAS_LIB_DIR) $(BLAS_LIBS) \
	-L$(FREDDO_LIB_DIR) $(FREDDO_LIBS)

.PHONY: clean
clean:
	rm -f --preserve-root bin/qr_right_looking
	make -C $(COREBLAS_ROOT) -f Makefile clean_all
	
