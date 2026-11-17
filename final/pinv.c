
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <lapacke.h>

#define M 30
#define N 40
#define K 30
#define ITERATIONS 1000

int main(void)
{
    lapack_complex_double A[M * N];
    lapack_complex_double Acopy[M * N];

    lapack_complex_double U[M * K];
    lapack_complex_double VT[K * N];

    lapack_complex_double Aplus[N * M];

    double S[K];
    double superb[K - 1];

    // 30 x 40 kompleks matris oluştur
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < M; i++) {
            A[i + j * M] =
                sin(i + j * 0.5) +
                I * cos(i * 0.3 + j);
        }
    }

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int iteration = 0; iteration < ITERATIONS; iteration++) {

        // SVD giriş matrisi değiştirildiği için kopyala
        for (int i = 0; i < M * N; i++)
            Acopy[i] = A[i];

        // SVD hesapla
        lapack_int info = LAPACKE_zgesvd(
            LAPACK_COL_MAJOR,
            'S',
            'S',
            M,
            N,
            Acopy,
            M,
            S,
            U,
            M,
            VT,
            K,
            superb
        );

        if (info != 0) {
            printf("SVD error: %d\n", info);
            return 1;
        }

        // Pseudoinverse hesapla: A+ = V * S+ * U^H
        double tolerance = 2.220446049250313e-16 * N * S[0];

        for (int j = 0; j < M; j++) {
            for (int i = 0; i < N; i++) {

                lapack_complex_double sum = 0.0;

                for (int l = 0; l < K; l++) {

                    if (S[l] > tolerance) {
                        sum += conj(VT[l + i * K])
                             * conj(U[j + l * M])
                             / S[l];
                    }
                }

                Aplus[i + j * N] = sum;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) * 1000.0 +
        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("Total time: %.6f ms\n", elapsed);
    printf("Average time: %.6f ms\n", elapsed / ITERATIONS);

    // Sonucun hesaplandığını doğrulamak için
    printf("Aplus[0] = %.6f + %.6fi\n",
           creal(Aplus[0]), cimag(Aplus[0]));

    return 0;
}

