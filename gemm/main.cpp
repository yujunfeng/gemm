
#include "00naive.h"
#include "01register.h"
#include "02cache.h"
#include "03blis_copy.h"
#include "util.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstdio>

#if defined(USE_MKL)
#include <mkl_cblas.h>
#define USE_CBLAS
#define CBLAS_IMPL "MKL"
#elif defined(USE_BLIS)
#include <blis/cblas.h>
#define USE_CBLAS
#define CBLAS_IMPL "BLIS"
#endif

using elem_type = float;


#ifdef USE_CBLAS
template <class T>
void cblas_gemm(
    int M, int N, int K, T alpha, T *A, int lda,
    T *B, int ldb, T beta, T *C, int ldc);

template <>
void cblas_gemm<float>(
    int M, int N, int K, float alpha, float *A, int lda,
    float *B, int ldb, float beta, float *C, int ldc)
{
    cblas_sgemm(
        CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K,
        alpha, A, lda, B, ldb, beta, C, ldc);
}

template <>
void cblas_gemm<double>(
    int M, int N, int K, double alpha, double *A, int lda,
    double *B, int ldb, double beta, double *C, int ldc)
{
    cblas_dgemm(
        CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K,
        alpha, A, lda, B, ldb, beta, C, ldc);
}
#endif

template <class T>
static void ref_gemm(
    int M, int N, int K, T alpha, T *A, int lda,
    T *B, int ldb, T beta, T *C, int ldc)
{
#ifdef USE_CBLAS
    cblas_gemm(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
#else
    //naive::gemm(
    cache_blocking_L3::gemm(
        M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
#endif
}

void transpose_test()
{
    alignas(32) float a[] = {
        0,  1,  2,  3,  4,  5,  6,  7,
        8,  9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62, 63,
    };
    auto row0 = _mm256_load_ps(a + 8 * 0);
    auto row1 = _mm256_load_ps(a + 8 * 1);
    auto row2 = _mm256_load_ps(a + 8 * 2);
    auto row3 = _mm256_load_ps(a + 8 * 3);
    auto row4 = _mm256_load_ps(a + 8 * 4);
    auto row5 = _mm256_load_ps(a + 8 * 5);
    auto row6 = _mm256_load_ps(a + 8 * 6);
    auto row7 = _mm256_load_ps(a + 8 * 7);

    __m256 __t0, __t1, __t2, __t3, __t4, __t5, __t6, __t7;
    __m256 __tt0, __tt1, __tt2, __tt3, __tt4, __tt5, __tt6, __tt7;
    __t0 = _mm256_unpacklo_ps(row0, row1);                          // a0 b0 a1 b1 a4 b4 a5 b5
    __t1 = _mm256_unpackhi_ps(row0, row1);                          // a2 b2 a3 b3 a6 b6 a7 b7
    __t2 = _mm256_unpacklo_ps(row2, row3);                          // c0 d0 c1 d1 c4 d4 c6 d6
    __t3 = _mm256_unpackhi_ps(row2, row3);                          // c2 d2 c3 d3 c6 d6 c7 d7
    __t4 = _mm256_unpacklo_ps(row4, row5);                          // e0 f0 e1 f1 e4 f4 e5 f5
    __t5 = _mm256_unpackhi_ps(row4, row5);                          // e2 f2 e3 f3 e6 f6 e7 f7
    __t6 = _mm256_unpacklo_ps(row6, row7);                          // g0 h0 g1 h1 g4 h4 g5 h5
    __t7 = _mm256_unpackhi_ps(row6, row7);                          // g2 h2 g3 h3 g6 h6 g7 h7

    __tt0 = _mm256_shuffle_ps(__t0, __t2, _MM_SHUFFLE(1, 0, 1, 0)); // a0 b0 c0 d0 a4 b4 c4 d4
    __tt1 = _mm256_shuffle_ps(__t0, __t2, _MM_SHUFFLE(3, 2, 3, 2)); // a1 b1 c1 d1 a5 b5 c5 d5
    __tt2 = _mm256_shuffle_ps(__t1, __t3, _MM_SHUFFLE(1, 0, 1, 0)); // a2 a2 c2 d2 a6 b6 c6 d6
    __tt3 = _mm256_shuffle_ps(__t1, __t3, _MM_SHUFFLE(3, 2, 3, 2)); // a3 b3 c3 d3 a7 b7 c7 d7
    __tt4 = _mm256_shuffle_ps(__t4, __t6, _MM_SHUFFLE(1, 0, 1, 0)); // e0 f0 g0 h0 e4 f4 g4 h4
    __tt5 = _mm256_shuffle_ps(__t4, __t6, _MM_SHUFFLE(3, 2, 3, 2)); // e1 f1 g1 h1 e5 f5 g5 h5
    __tt6 = _mm256_shuffle_ps(__t5, __t7, _MM_SHUFFLE(1, 0, 1, 0)); // e2 f2 g2 h2 e6 f6 g6 h6
    __tt7 = _mm256_shuffle_ps(__t5, __t7, _MM_SHUFFLE(3, 2, 3, 2)); // e3 f3 g3 h3 e7 f7 g7 h7

    row0 = _mm256_permute2f128_ps(__tt0, __tt4, 0x20);              // a0 b0 c0 d0 e0 f0 g0 h0
    row1 = _mm256_permute2f128_ps(__tt1, __tt5, 0x20);              // a1 b1 c1 d1 e1 f1 g1 h1
    row2 = _mm256_permute2f128_ps(__tt2, __tt6, 0x20);              // a2 b2 c2 d2 e2 f2 g2 h2
    row3 = _mm256_permute2f128_ps(__tt3, __tt7, 0x20);              // a3 b3 c3 d3 e3 f3 g3 h3
    row4 = _mm256_permute2f128_ps(__tt0, __tt4, 0x31);              // a4 b4 c4 d4 e4 f4 g4 h4
    row5 = _mm256_permute2f128_ps(__tt1, __tt5, 0x31);              // a5 b5 c5 d5 e5 f5 g5 h5
    row6 = _mm256_permute2f128_ps(__tt2, __tt6, 0x31);              // a6 b6 c6 d6 e6 f6 g6 h6 
    row7 = _mm256_permute2f128_ps(__tt3, __tt7, 0x31);              // a7 b7 c7 d7 e7 f7 g7 h7

    alignas(32) float b[64];
    _mm256_store_ps(b + 8 * 0, row0);
    _mm256_store_ps(b + 8 * 1, row1);
    _mm256_store_ps(b + 8 * 2, row2);
    _mm256_store_ps(b + 8 * 3, row3);
    _mm256_store_ps(b + 8 * 4, row4);
    _mm256_store_ps(b + 8 * 5, row5);
    _mm256_store_ps(b + 8 * 6, row6);
    _mm256_store_ps(b + 8 * 7, row7);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%2.0f ", b[8 * i + j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    int size = (argc >= 2) ? atoi(argv[1]) : 96;

    if (0 && size % 32 != 0) {
        std::fprintf(stderr,
            "error: matrix size must be a multiple of 32\n");
        return 1;
    }

    int M = size;
    int N = size;
    int K = size;
#if 0
    int lda = K;
    int ldb = N;
    int ldc = K;
#else
    constexpr int PAD = 0; // 64 / sizeof(elem_type);

    // add 64byte-padding
    int lda = K + PAD;
    int ldb = N + PAD;
    int ldc = N + PAD;
#endif
    auto alpha = elem_type(1.0); // elem_type(4.0);
    auto beta = elem_type(1.0); // elem_type(0.25);

    int align = sizeof(__m256);
    auto A = make_aligned_array<elem_type>(M * lda, align, elem_type(2.0));
    auto B = make_aligned_array<elem_type>(K * ldb, align, elem_type(0.5));
    auto C = make_aligned_array<elem_type>(M * ldc, align, elem_type(0.0));
    auto result_C = make_aligned_array<elem_type>(M * ldc, align, elem_type(0.0));
    
    auto buf_size = 16 * 1024 * 1024 / sizeof(elem_type);
    auto buf = make_aligned_array<elem_type>(buf_size, align, 0.1);

    bench_params<elem_type> bp = {
        M, N, K, alpha, A.get(), lda, B.get(), ldb, beta, 
        C.get(), ldc, result_C.get(), buf.get(), int(buf_size),
    };

    register_bench::perform(bp);

    // make result_C
    ref_gemm(M, N, K, alpha, A.get(), lda, B.get(), ldb, beta, result_C.get(), ldc);

#ifdef USE_CBLAS
    // MKL warmup
    cblas_gemm(M, N, K, alpha, A.get(), lda, B.get(), ldb, beta, C.get(), ldc);

    // MKL
    benchmark(CBLAS_IMPL, bp, cblas_gemm<elem_type>);
#endif

#if 1
    if (size <= 512) {
        // 0-0. Naive implementation
        benchmark("naive", bp, naive::gemm<elem_type>);
    }
#endif

#ifdef USE_AVX
#if 0
    if (size <= 768) {
        // 0-1. Naive AVX implementation
        benchmark("naive_avx", bp, naive_avx::gemm);

        // 1-0. Register blocking (but spilled) AVX implementation
        benchmark("register_avx_0", bp, register_avx_0::gemm);

        // 1-1. Register blocking AVX implementation
        benchmark("register_avx_1", bp, register_avx_1::gemm);

        // 1-2. Register blocking AVX implementation (blocking with K)
        benchmark("register_avx_2", bp, register_avx_2::gemm);
    }

    // 2-1. cache-oblivious implementation with 1-2.
    benchmark("cache_oblivious", bp, cache_oblivious::gemm);

    // 2-2. L1-cache blocking implementation with 1-2.
    benchmark("L1 blocking", bp, cache_blocking_L1::gemm);

    // 2-3. L2-cache blocking implementation with 2-2.
    benchmark("L2 blocking", bp, cache_blocking_L2::gemm);
#endif
    // 2-3. L2-cache blocking implementation with 2-2.
    //benchmark("L3 blocking", bp, cache_blocking_L3::gemm);
#endif
    // 3-1. BLIS-based implementation
    benchmark("BLIS_naive", bp, blis_copy<blis_opt::nopack>::gemm);

    // 3-2. BLIS-based implementation w/ copy optimization on L1
    benchmark("BLIS_packL1", bp, blis_copy<blis_opt::packL1>::gemm);

    // 3-3. BLIS-based implementation w/ copy optimization on L1/L2
    benchmark("BLIS_packL2", bp, blis_copy<blis_opt::packL2>::gemm);

    // 3-3. BLIS-based implementation w/ copy optimization on L1/L2
    benchmark("BLIS_packL3", bp, blis_copy<blis_opt::packL3>::gemm);
    
    return 0;
}
