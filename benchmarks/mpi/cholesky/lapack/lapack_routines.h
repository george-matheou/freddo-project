/*
 * lapack_routines.h
 *
 *  Created on: Jun 27, 2017
 *      Author: geomat
 */

#ifndef CHOLESKY_LAPACK_LAPACK_ROUTINES_H_
#define CHOLESKY_LAPACK_LAPACK_ROUTINES_H_


extern "C" {
#include "f2c.h"
#include "clapack.h"

//#ifdef 	SINGLE_PRECISION

	int ssyrk_(char *uplo, char *trans, integer *n, integer *k, real *alpha, real *a, integer *lda, real *beta, real *c, integer *ldc);

	int sgemm_(char *transa, char *transb, integer *m, integer * n, integer *k, real *alpha, real *a, integer *lda, real *b, integer *ldb, real *beta,
			real *c, integer *ldc);

	int spotf2_(char *uplo, integer *n, real *a, integer * lda, integer *info);

	int strsm_(char *side, char *uplo, char *transa, char *diag, integer *m, integer *n, real *alpha, real *a, integer * lda, real *b, integer *ldb);
//#else
	int dsyrk_(char *uplo, char *trans, integer *n, integer *k, doublereal *alpha, doublereal *a, integer *lda, doublereal *beta, doublereal *c,
		integer *ldc);

	int dgemm_(char *transa, char *transb, integer *m, integer * n, integer *k, doublereal *alpha, doublereal *a, integer *lda, doublereal *b,
		integer *ldb, doublereal *beta, doublereal *c, integer *ldc);

	int dpotf2_(char *uplo, integer *n, doublereal *a, integer * lda, integer *info);

	int dtrsm_(char *side, char *uplo, char *transa, char *diag, integer *m, integer *n, doublereal *alpha, doublereal *a, integer * lda,
		doublereal *b, integer *ldb);
//#endif
}


#endif /* CHOLESKY_LAPACK_LAPACK_ROUTINES_H_ */
