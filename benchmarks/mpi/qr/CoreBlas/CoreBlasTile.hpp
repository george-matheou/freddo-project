/*
 * CoreBlas.hpp
 *
 *  Created on: 2015/09/11
 *      Author: stomo
 */

#ifndef COREBLASTILE_HPP_
#define COREBLASTILE_HPP_

#include <iostream>
#include <plasma.h>
#include <core_blas.h>
#include <cassert>
#include "TileMatrix/TileMatrix.h"

#ifndef __DEF__MIN__
#define __DEF__MIN__

#define min(a,b) (((a)<(b)) ? (a) : (b))

#endif // __DEF__MIN__

#ifndef  __DEF_MAX__
#define  __DEF_MAX__
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

/*
 * GEQRT conputes a QR factorization of a tile A: A = Q * R
 *
 * @param A (M x N) tile matrix
 * @param T (IB x N) upper triangular block reflector
 */
//template<typename Type>
void GEQRT_D(BMatrix<double> *A, BMatrix<double> *T) {
	const int M = A->m();
	const int N = A->n();
	const int IB = A->ib();
	const int LDA = A->m();
	const int LDT = T->m();

	const int NB = max(LDA, LDT);

	double* WORK = new double[IB * NB];
	double* TAU = new double[NB];

	CORE_dgeqrt(M, N, IB, A->top(), LDA, T->top(), LDT, TAU, WORK);

	delete[] WORK;
	delete[] TAU;
}

/*
 * GEQRT conputes a QR factorization of a tile A: A = Q * R
 *
 * @param A (M x N) tile matrix
 * @param T (IB x N) upper triangular block reflector
 */
void GEQRT_S(BMatrix<float> *A, BMatrix<float> *T) {
	const int M = A->m();
	const int N = A->n();
	const int IB = A->ib();
	const int LDA = A->m();
	const int LDT = T->m();

	const int NB = max(LDA, LDT);

	float* WORK = new float[IB * NB];
	float* TAU = new float[NB];

	CORE_sgeqrt(M, N, IB, A->top(), LDA, T->top(), LDT, TAU, WORK);

	delete[] WORK;
	delete[] TAU;
}

/*
 * TSQRT conputes a QR factorization of a rectangular matrix formed by cupling (N x N) upper triangular tile A1 on top of (M x N) tile A2
 *
 * @param A1 (N x N) upper triangular tile matrix
 * @param A2 (M x N) tile matrix
 * @param T (IB x N) upper triangular block reflector
 */
void TSQRT_D(BMatrix<double> *A1, BMatrix<double> *A2, BMatrix<double> *T) {
	const int M = A2->m();
	const int N = A1->n();

	assert(N == (int ) A2->n());

	const int IB = A1->ib();
	const int LDA1 = A1->m();
	const int LDA2 = A2->m();
	const int LDT = T->m();

	const int NB = max(LDA1, LDT);

	double* WORK = new double[IB * NB];
	double* TAU = new double[NB];

	CORE_dtsqrt(M, N, IB, A1->top(), LDA1, A2->top(), LDA2, T->top(), LDT, TAU, WORK);

	delete[] WORK;
	delete[] TAU;
}

/*
 * TSQRT conputes a QR factorization of a rectangular matrix formed by cupling (N x N) upper triangular tile A1 on top of (M x N) tile A2
 *
 * @param A1 (N x N) upper triangular tile matrix
 * @param A2 (M x N) tile matrix
 * @param T (IB x N) upper triangular block reflector
 */
void TSQRT_S(BMatrix<float> *A1, BMatrix<float> *A2, BMatrix<float> *T) {
	const int M = A2->m();
	const int N = A1->n();

	assert(N == (int ) A2->n());

	const int IB = A1->ib();
	const int LDA1 = A1->m();
	const int LDA2 = A2->m();
	const int LDT = T->m();

	const int NB = max(LDA1, LDT);

	float* WORK = new float[IB * NB];
	float* TAU = new float[NB];

	CORE_stsqrt(M, N, IB, A1->top(), LDA1, A2->top(), LDA2, T->top(), LDT, TAU, WORK);

	delete[] WORK;
	delete[] TAU;
}

/*
 * LARFB updates (M x N) tile C with the transformation formed with A and T
 *
 * @param[in] side
 *		@arg PlasmaLeft:  apply transformation from the left
 *		@arg PlasmaRight: apply transformation from the right
 *
 * @param[in] trans
 *		@arg PlasmaNoTrans: no transpose the matrix
 *		@arg PlasmaTrans: transpose the matrix
 *
 * @param[in] A (LDA x K) tile matrix
 * @param[in] T (IB x K) upper triangular block reflector
 * @param[in,out] C (M x N) tile matrix
 */
void LARFB_D(PLASMA_enum side, PLASMA_enum trans, BMatrix<double> *A, BMatrix<double> *T, BMatrix<double> *C) {
	assert((side == PlasmaLeft) || (side == PlasmaRight));
	assert((trans == PlasmaTrans) || (trans == PlasmaNoTrans));

	const int M = C->m();
	const int N = C->n();
	const int K = A->n();

	if (side == PlasmaLeft)
		assert(M >= K);
	else
		// (side == PlasmaRight)
		assert(N >= K);

	const int IB = A->ib();
	const int LDA = A->m();
	const int LDT = T->m();
	const int LDC = C->m();

	const int NB = max(LDA, LDT);

	double* WORK = new double[IB * NB];

	CORE_dormqr(side, trans, M, N, K, IB, A->top(), LDA, T->top(), LDT, C->top(), LDC, WORK, NB);

	delete[] WORK;
}

/*
 * LARFB updates (M x N) tile C with the transformation formed with A and T
 *
 * @param[in] side
 *		@arg PlasmaLeft:  apply transformation from the left
 *		@arg PlasmaRight: apply transformation from the right
 *
 * @param[in] trans
 *		@arg PlasmaNoTrans: no transpose the matrix
 *		@arg PlasmaTrans: transpose the matrix
 *
 * @param[in] A (LDA x K) tile matrix
 * @param[in] T (IB x K) upper triangular block reflector
 * @param[in,out] C (M x N) tile matrix
 */
void LARFB_S(PLASMA_enum side, PLASMA_enum trans, BMatrix<float> *A, BMatrix<float> *T, BMatrix<float> *C) {
	assert((side == PlasmaLeft) || (side == PlasmaRight));
	assert((trans == PlasmaTrans) || (trans == PlasmaNoTrans));

	const int M = C->m();
	const int N = C->n();
	const int K = A->n();

	if (side == PlasmaLeft)
		assert(M >= K);
	else
		// (side == PlasmaRight)
		assert(N >= K);

	const int IB = A->ib();
	const int LDA = A->m();
	const int LDT = T->m();
	const int LDC = C->m();

	const int NB = max(LDA, LDT);

	float* WORK = new float[IB * NB];

	CORE_sormqr(side, trans, M, N, K, IB, A->top(), LDA, T->top(), LDT, C->top(), LDC, WORK, NB);

	delete[] WORK;
}

/*
 * SSRFB updates (M1 x N1) tile C1 and (M2 x N2) tile C2 with the transformation formed with A and T
 *
 * @param[in] side
 *		@arg PlasmaLeft:  apply transformation from the left
 *		@arg PlasmaRight: apply transformation from the right
 *
 * @param[in] trans
 *		@arg PlasmaNoTrans: no transpose the matrix
 *		@arg PlasmaTrans: transpose the matrix
 *
 * @param[in] A (LDA x K) tile matrix
 * @param[in] T (IB x K) upper triangular block reflector
 * @param[in,out] C1 (M1 x N1) tile matrix
 * @param[in,out] C2 (M2 x N2) tile matrix
 */
void SSRFB_D(PLASMA_enum side, PLASMA_enum trans, BMatrix<double> *A, BMatrix<double> *T, BMatrix<double> *C1, BMatrix<double> *C2) {
	assert((side == PlasmaLeft) || (side == PlasmaRight));
	assert((trans == PlasmaTrans) || (trans == PlasmaNoTrans));

	const int M1 = C1->m();
	const int M2 = C2->m();

	if (side == PlasmaRight)
		assert(M2 == M1);

	const int N1 = C1->n();
	const int N2 = C2->n();

	if (side == PlasmaLeft)
		assert(N2 == N1);

	const int K = A->n();

	const int IB = C1->ib();
	const int LDA1 = C1->m();
	const int LDA2 = C2->m();
	const int LDV = A->m();
	const int LDT = T->m();

	int LDWORK;
	if (side == PlasmaLeft)
		LDWORK = IB;
	else
		// side == PlasmaRight
		LDWORK = M1;

	int WSIZE;
	if (side == PlasmaLeft)
		WSIZE = N1;
	else
		// side == PlasmaRight
		WSIZE = IB;

	double* WORK = new double[LDWORK * WSIZE];

	CORE_dtsmqr(side, trans, M1, N1, M2, N2, K, IB, C1->top(), LDA1, C2->top(), LDA2, A->top(), LDV, T->top(), LDT, WORK, LDWORK);

	delete[] WORK;
}

/*
 * SSRFB updates (M1 x N1) tile C1 and (M2 x N2) tile C2 with the transformation formed with A and T
 *
 * @param[in] side
 *		@arg PlasmaLeft:  apply transformation from the left
 *		@arg PlasmaRight: apply transformation from the right
 *
 * @param[in] trans
 *		@arg PlasmaNoTrans: no transpose the matrix
 *		@arg PlasmaTrans: transpose the matrix
 *
 * @param[in] A (LDA x K) tile matrix
 * @param[in] T (IB x K) upper triangular block reflector
 * @param[in,out] C1 (M1 x N1) tile matrix
 * @param[in,out] C2 (M2 x N2) tile matrix
 */
void SSRFB_S(PLASMA_enum side, PLASMA_enum trans, BMatrix<float> *A, BMatrix<float> *T, BMatrix<float> *C1, BMatrix<float> *C2) {
	assert((side == PlasmaLeft) || (side == PlasmaRight));
	assert((trans == PlasmaTrans) || (trans == PlasmaNoTrans));

	const int M1 = C1->m();
	const int M2 = C2->m();

	if (side == PlasmaRight)
		assert(M2 == M1);

	const int N1 = C1->n();
	const int N2 = C2->n();

	if (side == PlasmaLeft)
		assert(N2 == N1);

	const int K = A->n();

	const int IB = C1->ib();
	const int LDA1 = C1->m();
	const int LDA2 = C2->m();
	const int LDV = A->m();
	const int LDT = T->m();

	int LDWORK;
	if (side == PlasmaLeft)
		LDWORK = IB;
	else
		// side == PlasmaRight
		LDWORK = M1;

	int WSIZE;
	if (side == PlasmaLeft)
		WSIZE = N1;
	else
		// side == PlasmaRight
		WSIZE = IB;

	float* WORK = new float[LDWORK * WSIZE];

	CORE_stsmqr(side, trans, M1, N1, M2, N2, K, IB, C1->top(), LDA1, C2->top(), LDA2, A->top(), LDV, T->top(), LDT, WORK, LDWORK);

	delete[] WORK;
}


// for TSQR

/*
 * TTQRT conputes a QR factorization of a rectangular matrix formed by cupling (N x N) upper triangular tile A1 on top of (M x N) upper trapezoidal tile A2
 *
 * @param A1 (N x N) upper triangular tile matrix
 * @param A2 (M x N) upper trapezoidal tile matrix
 * @param T (IB x N) upper triangular block reflector
 */
void TTQRT(BMatrix<double> *A1, BMatrix<double> *A2, BMatrix<double> *T) {
	const int M = A2->m();
	const int N = A1->n();

	assert((size_t ) N == A2->n());

	const int IB = A1->ib();
	const int LDA1 = A1->m();
	const int LDA2 = A2->m();
	const int LDT = T->m();

	const int NB = max(LDA1, LDT);

	double* WORK = new double[IB * NB];
	double* TAU = new double[NB];

	CORE_dttqrt(M, N, IB, A1->top(), LDA1, A2->top(), LDA2, T->top(), LDT, TAU, WORK);

	delete[] WORK;
	delete[] TAU;
}

/*
 * TTMQR updates (M1 x N1) tile C1 and (M2 x N2) tile C2 with the transformation formed with A and T
 *
 * @param[in] side
 *		@arg PlasmaLeft:  apply transformation from the left
 *		@arg PlasmaRight: apply transformation from the right
 *
 * @param[in] trans
 *		@arg PlasmaNoTrans: no transpose the matrix
 *		@arg PlasmaTrans: transpose the matrix
 *
 * @param[in] A (LDA x K) tile matrix
 * @param[in] T (IB x K) upper triangular block reflector
 * @param[in,out] C1 (M1 x N1) tile matrix
 * @param[in,out] C2 (M2 x N2) tile matrix
 */
void TTMQR(PLASMA_enum side, PLASMA_enum trans, BMatrix<double> *A, BMatrix<double> *T, BMatrix<double> *C1, BMatrix<double> *C2) {
	assert((side == PlasmaLeft) || (side == PlasmaRight));
	assert((trans == PlasmaTrans) || (trans == PlasmaNoTrans));

	const int M1 = C1->m();
	const int M2 = C2->m();

	const int N1 = C1->n();
	const int N2 = C2->n();

	const int K = A->n();

	const int IB = C1->ib();
	const int LDA1 = C1->m();
	const int LDA2 = C2->m();
	const int LDV = A->m();
	const int LDT = T->m();

	int LDWORK;
	if (side == PlasmaLeft)
		LDWORK = IB;
	else
		// side == PlasmaRight
		LDWORK = M1;

	int WSIZE;
	if (side == PlasmaLeft)
		WSIZE = N1;
	else
		// side == PlasmaRight
		WSIZE = IB;

	double* WORK = new double[LDWORK * WSIZE];

	CORE_dttmqr(side, trans, M1, N1, M2, N2, K, IB, C1->top(), LDA1, C2->top(), LDA2, A->top(), LDV, T->top(), LDT, WORK, LDWORK);

	delete[] WORK;
}

/*
 * dorgqr: genarates (M x N) orthogonal matrix Q: A = Q x R
 *
 * @param[in] A tile matrix
 * @param[in] T tile matrix
 * @param[in] Q tile matirx
 *
 */
void dorgqr(const TileMatrix<double> A, const TileMatrix<double> T, TileMatrix<double>& Q) {
	assert(A.M() == Q.M());

	const int aMT = A.mt();
	const int aNT = A.nt();
	const int qMT = Q.mt();
	const int qNT = Q.nt();

	for (int tk = min(aMT, aNT) - 1; tk + 1 >= 1; tk--) {
		for (int ti = qMT - 1; ti > tk; ti--) {

			for (int tj = tk; tj < qNT; tj++) {
				SSRFB_D(PlasmaLeft, PlasmaNoTrans, A(ti, tk), T(ti, tk), Q(tk, tj), Q(ti, tj));
			}
		}

		for (int tj = tk; tj < qNT; tj++) {
			LARFB_D(PlasmaLeft, PlasmaNoTrans, A(tk, tk), T(tk, tk), Q(tk, tj));
		}
	}
}

// for Cholesky
//template<typename Type>
//void POTRF(BMatrix<Type> *A);
//
//template<typename Type>
//void SYRK(BMatrix<Type> *A, BMatrix<Type> *B);
//
//template<typename Type>
//void TRSM(BMatrix<Type> *A, BMatrix<Type> *B);
//
//template<typename Type>
//void GEMM(BMatrix<Type> *A, BMatrix<Type> *B, BMatrix<Type> *C);

#endif /* COREBLASTILE_HPP_ */
