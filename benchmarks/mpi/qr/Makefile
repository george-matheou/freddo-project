include ../Makefile.in
FREDDO_LIB_DIR		=../../../install_freddo/lib
FREDDO_INC_DIR		=../../../install_freddo/include

## LAPACK paths and libraries
LAPACK_DIR		= /opt/lapack-3.6.1
LIBS_PATHS		= -L$(LAPACK_DIR) -L$(LAPACK_DIR)/lib -L$(FREDDO_LIB_DIR)
LIBS			=  -llapack -lcblas -lblas -lgfortran -lm -lfreddo  # -pthread -llapacke 
INCLUDES		= -I$(LAPACK_DIR)/CBLAS/include -I$(LAPACK_DIR)/LAPACKE/include -I$(FREDDO_INC_DIR)

## Plasma Libraries
PLASMA_ROOT 	= /home/geomat/plasma
PLASMA_INC_DIR 	= $(PLASMA_ROOT)/include
PLASMA_LIB_DIR 	= $(PLASMA_ROOT)/lib
PLASMA_LIBS 	= -lplasma -lcoreblas #-lquark
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
BLAS_LIBS    =  -llapacke -llapack -lcblas -lblas -lgfortran -lm #
# FREDDO libraries
FREDDO_LIBS 	= -lfreddo -pthread

EXECS=core_blas_lib bin/qr_right_looking 

ifeq (openmpi,$(MAKECMDGOALS))
      CXX=$(CXX_openmpi)
else ifeq (mvapich,$(MAKECMDGOALS))
	  CXX=$(CXX_mvapich)   
else ifeq (mpich,$(MAKECMDGOALS))
	  CXX=$(CXX_mpich)     
else 
      CXX=$(CXX_mpidefault)
endif

.PHONY: openmpi
openmpi:$(EXECS)

.PHONY: mvapich	
mvapich:$(EXECS)

.PHONY: mpich	
mpich:$(EXECS)

.PHONY: mpidefault	
mpidefault:$(EXECS)

core_blas_lib:
	make -C $(COREBLAS_ROOT)

bin/qr_right_looking: qr_right_looking.cpp
	$(CXX) $(CXXFLAGS) -o  bin/qr_right_looking qr_right_looking.cpp  \
	$(BLAS_INC_DIR) -I$(PLASMA_INC_DIR) -I$(COREBLAS_INC_DIR) -I$(TILE_MATRIX_INC_DIR) -I$(FREDDO_INC_DIR) \
	-L$(COREBLAS_LIB_DIR) $(COREBLAS_LIBS) \
	-L$(PLASMA_LIB_DIR) $(PLASMA_LIBS) \
	$(BLAS_LIB_DIR) $(BLAS_LIBS) \
	-L$(FREDDO_LIB_DIR) $(FREDDO_LIBS)

clean:
	rm --preserve-root bin/qr_right_looking
	rm --preserve-root -f $(COREBLAS_ROOT)/*.o  $(COREBLAS_ROOT)/*.a
	
