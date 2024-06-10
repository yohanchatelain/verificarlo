void vecfma(float *restrict c, float *restrict a, float *restrict b,
            const int n) {
  for (int i = 0; i < n; i++)
    c[i] += a[i] * b[i];
}