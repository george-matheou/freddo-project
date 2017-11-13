//============================================================================
// Name        : CoreBlas.cpp
// Author      : T. Suzuki
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================


#include "CoreBlasTile.hpp"

/*
 * POTRF Computes the Cholesky factorization of a symmetric positive definite matrix A
 *
 * @param[in,out] A
 *
 */
/*void POTRF( BMatrix *A )
 {
 const int M = A->m();
 const int N = A->n();
 const int LDA = A->m();

 int info;

 CORE_dpotrf( PlasmaLower,
 M,
 A->top(), LDA,
 &info );
 }*/

/*
 * SYRK Performs one of the hermitian rank k operations
 *
 *
 * @param[in] A
 * @param[in,out] C
 *
 *
 *    C = \alpha [ A \times A' ] + \beta C ,
 *
 *
 *  where alpha and beta are real scalars, C is an N-by-N hermitian
 *  matrix and A is an N-by-K matrix.
 *
 * @param[in] uplo
 *          = PlasmaLower: Lower triangle of C is stored.
 *
 * @param[in] trans
 *          = PlasmaNoTrans:   A is not transposed;
 *
 * @param[in] N
 *          N specifies the order of the matrix C. N must be at least zero.
 *
 * @param[in] K
 *          K specifies the number of columns of the matrix A.
 *
 * @param[in] alpha
 *          alpha specifies the scalar alpha.
 *
 * @param[in] A
 *          A is a LDA-by-K matrix.
 *
 * @param[in] LDA
 *          The leading dimension of the array A. LDA must be at least max( 1, N )
 *
 * @param[in] beta
 *          beta specifies the scalar beta
 *
 * @param[in,out] C
 *          C is a LDC-by-N matrix.
 *          On exit, the array uplo part of the matrix is overwritten
 *          by the uplo part of the updated matrix.
 *
 * @param[in] LDC
 *          The leading dimension of the array C. LDC >= max( 1, N ).
 *
 */
/*void SYRK( BMatrix *A, BMatrix *C )
 {

 const int M = A->m();

 assert( M == C->m() );

 CORE_dsyrk(
 PlasmaLower, PlasmaNoTrans,
 M, M,
 -1.0, A->top(), M,
 1.0, C->top(), M);
 }*/

/*
 *  TRSM - Computes triangular solve X*A = B.
 *
 *******************************************************************************
 *
 * @param[in] side
 *          = PlasmaRight: X*A = B
 *
 * @param[in] uplo
 *          = PlasmaLower: Lower triangle of A is stored.
 *
 * @param[in] transA
 *          = PlasmaTrans:     A is not transposed;
 *
 * @param[in] diag
 *          = PlasmaNonUnit: A is non unit;
 *
 * @param[in] M
 *          The order of the matrix A. M >= 0.
 *
 * @param[in] N
 *          The number of right hand sides, i.e., the number of columns of the matrix B. N >= 0.
 *
 * @param[in] alpha
 *          alpha specifies the scalar alpha.
 *
 * @param[in] A
 *          The triangular matrix A. If uplo = PlasmaUpper, the leading M-by-M upper triangular
 *          part of the array A contains the upper triangular matrix, and the strictly lower
 *          triangular part of A is not referenced. If uplo = PlasmaLower, the leading M-by-M
 *          lower triangular part of the array A contains the lower triangular matrix, and the
 *          strictly upper triangular part of A is not referenced. If diag = PlasmaUnit, the
 *          diagonal elements of A are also not referenced and are assumed to be 1.
 *
 * @param[in] LDA
 *          The leading dimension of the array A. LDA >= max(1,M).
 *
 * @param[in,out] B
 *          On entry, the M-by-N right hand side matrix B.
 *          On exit, if return value = 0, the M-by-N solution matrix X.
 *
 * @param[in] LDB
 *          The leading dimension of the array B. LDB >= max(1,M).
 */
/*void TRSM( BMatrix *A, BMatrix *B )
 {
 const int M = A->m();
 const int N = B->n();

 assert ( M == B->m() );

 CORE_dtrsm( PlasmaRight, PlasmaLower, PlasmaTrans, PlasmaNonUnit,
 M, N,
 1.0,
 A->top(), M,
 B->top(), M );
 }*/

/*
 *  *  GEMM - Performs one of the matrix-matrix operations
 *
 *    C = \alpha [ A \times B' ] + \beta C ,
 *
 *  alpha and beta are scalars, and A, B and C  are matrices, with op( A )
 *  an m by k matrix, op( B ) a k by n matrix and C an m by n matrix.
 *
 *******************************************************************************
 *
 * @param[in] transA
 *          Specifies whether the matrix A is transposed, not transposed or ugate transposed:
 *          = PlasmaNoTrans:   A is not transposed;
 *          = PlasmaTrans:     A is transposed;
 *          = PlasmaTrans: A is ugate transposed.
 *
 * @param[in] transB
 *          Specifies whether the matrix B is transposed, not transposed or ugate transposed:
 *          = PlasmaNoTrans:   B is not transposed;
 *          = PlasmaTrans:     B is transposed;
 *          = PlasmaTrans: B is ugate transposed.
 *
 * @param[in] M
 *          M specifies the number of rows of the matrix op( A ) and of the matrix C. M >= 0.
 *
 * @param[in] N
 *          N specifies the number of columns of the matrix op( B ) and of the matrix C. N >= 0.
 *
 * @param[in] K
 *          K specifies the number of columns of the matrix op( A ) and the number of rows of
 *          the matrix op( B ). K >= 0.
 *
 * @param[in] alpha
 *          alpha specifies the scalar alpha
 *
 * @param[in] A
 *          A is a LDA-by-ka matrix, where ka is K when  transA = PlasmaNoTrans,
 *          and is  M  otherwise.
 *
 * @param[in] LDA
 *          The leading dimension of the array A. LDA >= max(1,M).
 *
 * @param[in] B
 *          B is a LDB-by-kb matrix, where kb is N when  transB = PlasmaNoTrans,
 *          and is  K  otherwise.
 *
 * @param[in] LDB
 *          The leading dimension of the array B. LDB >= max(1,N).
 *
 * @param[in] beta
 *          beta specifies the scalar beta
 *
 * @param[in,out] C
 *          C is a LDC-by-N matrix.
 *          On exit, the array is overwritten by the M by N matrix ( alpha*op( A )*op( B ) + beta*C )
 *
 * @param[in] LDC
 *          The leading dimension of the array C. LDC >= max(1,M).
 *

 */
/*void GEMM( BMatrix *A, BMatrix *B, BMatrix *C )
 {

 CORE_dgemm( PlasmaNoTrans, PlasmaTrans,
 tempmn, A.nb, A.nb,
 -1.0,
 A(m, n), ldam,
 A(k, n), ldak,
 1.0, A(m, k), ldam);

 }*/
