#include "amgif.h"
#include "linalg.h"

#include <iostream>
#include <chrono>


using std::chrono::high_resolution_clock;
using std::chrono::duration;
using std::chrono::duration_cast;


double deviationAMGIF(gsl_vector *y, gsl_vector *yi) {
    // 1-norm distance  
    gsl_vector *sub = gsl_vector_alloc(y->size);
    gsl_vector_memcpy(sub, y);
    gsl_vector_sub(sub, yi);
    
    double dist = gsl_blas_dasum(sub);

    gsl_vector_free(sub);

    return dist;
}

pair<gsl_vector *, gsl_vector*> computeAMGIF(
        vector<mat_operator>& C,
        gsl_function *me,
        gsl_function *pe,
        mat_operator& L,
        double alpha,
        double beta,
        double tau,
        double eps
) {
    gsl_vector *di, *x, *yi, *y, *rhs;
    gsl_matrix *A, *B, *lhs;
    size_t length, mu;
    double err;

    length = basis_length();

    di = coords(me);
    yi = coords(pe);

    x = gsl_vector_alloc(length);
    y = yi;

    A = gsl_matrix_alloc(length, length);
    B = gsl_matrix_alloc(length, length);
    
    lhs = gsl_matrix_alloc(length, length);
    rhs = gsl_vector_alloc(length);

    auto div = gsl_matrix_view_vector(di, length, 1);
    auto xv = gsl_matrix_view_vector(x, length, 1);
    auto yv = gsl_matrix_view_vector(y, length, 1);
    auto rhsv = gsl_matrix_view_vector(rhs, length, 1);

    size_t iter = 1;
    do {
        auto t1 = high_resolution_clock::now();

        // Minimize for Y

        for (mu = 0; mu < length; ++mu) {
            auto row = gsl_matrix_submatrix(A, mu, 0, 1, length);
            
            gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, &yv.matrix, C[mu].get(), 0.0, &row.matrix);
        }

        gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, L.get(), L.get(), 0.0, lhs);
        gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, A, A, alpha, lhs);

        gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, A, &div.matrix, 0.0, &rhsv.matrix);

        gsl_linalg_cholesky_decomp1(lhs);
        gsl_linalg_cholesky_solve(lhs, rhs, x);

        // Minimize for X

        for (mu = 0; mu < length; ++mu) {
            auto row = gsl_matrix_row(B, mu);
            auto rowT = gsl_matrix_view_vector(&row.vector, length, 1);

            gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, C[mu].get(), &xv.matrix, 0.0, &rowT.matrix);
        }
        
        gsl_matrix_set_identity(lhs);
        gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, B, B, beta + tau, lhs);

        gsl_vector_memcpy(rhs, yi);
        gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, B, &div.matrix, tau, &rhsv.matrix);

        gsl_linalg_cholesky_decomp1(lhs);
        gsl_linalg_cholesky_solve(lhs, rhs, y);

        auto t2 = high_resolution_clock::now();

        err = deviationAMGIF(y, yi);

        if (iter % 100 == 0) {
            auto dur = duration_cast<duration<double>>(t2 - t1);
            std::cout << "  * Iteration " << iter << " with error " << err
                      << " (" << dur.count() << ")" << std::endl;
        }
        
        iter++;
    } while (err > eps);

    std::cout << "  * Converged in " << iter << " iterations with final error " << err << std::endl;

    gsl_matrix_free(A);
    gsl_matrix_free(B);
    gsl_matrix_free(lhs);
    gsl_vector_free(rhs);

    return pair<gsl_vector *, gsl_vector *>(x, y);
}
