/* Tests that PRISM SR virtual precision reduces numerical precision at the
 * correct bit position for +, *, fma, and / operations.
 *
 * The do_*_f32/f64/v4/v2d computation functions are PRISM-instrumented via
 * include-functions.txt.  The analysis code (main, check_*) is compiled with
 * regular IEEE arithmetic so statistical checks are unaffected by SR.
 *
 * At virtual precision p (float: 2..24, double: 2..53):
 *
 *   BRACKETS:   exactly 2 distinct SR values appear, differing by 2^noise_bits
 *               (= 1 ULP at precision p) in the integer representation.
 *   STOCHASTIC: both brackets are observed in N samples.
 *   UNBIASED:   sample mean (in long double) converges to the exact result
 *               within 5σ.
 *
 * For vector mode: per-lane BRACKETS/STOCHASTIC plus lane-independence check.
 *
 * Usage: ./test_accuracy <float|double|vector|vector-double> <div|add|mul|fma>
 *                        <precision> <a> <b> [<c>]
 *
 * Operands come from argv to prevent compile-time constant folding.
 * c defaults to 0 for non-fma operations.
 */

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static constexpr int N = 300;

/* GCC/Clang vector extensions. */
using float4 = __attribute__((__vector_size__(4 * sizeof(float)))) float;
using double2 = __attribute__((__vector_size__(2 * sizeof(double)))) double;

/* ---- PRISM-instrumented computation functions ---------------------------
 * Only these are listed in include-functions.txt; all other functions use
 * regular IEEE arithmetic.
 *
 * The asm volatile barrier prevents the compiler from inferring memory(none),
 * which would allow it to hoist and CSE repeated calls with the same arguments
 * out of the sampling loop.  After PRISM instruments the FP op, the function
 * reads RNG and virtual_precision state, so the barrier is semantically
 * correct. */
#define COMPUTATION_FN __attribute__((noinline))
#define ASM_BARRIER __asm__ volatile ("" ::: "memory")

COMPUTATION_FN float  do_div_f32(float  a, float  b          ) { ASM_BARRIER; return a / b; }
COMPUTATION_FN float  do_add_f32(float  a, float  b          ) { ASM_BARRIER; return a + b; }
COMPUTATION_FN float  do_mul_f32(float  a, float  b          ) { ASM_BARRIER; return a * b; }
COMPUTATION_FN float  do_fma_f32(float  a, float  b, float  c) { ASM_BARRIER; return std::fma(a, b, c); }

COMPUTATION_FN double do_div_f64(double a, double b          ) { ASM_BARRIER; return a / b; }
COMPUTATION_FN double do_add_f64(double a, double b          ) { ASM_BARRIER; return a + b; }
COMPUTATION_FN double do_mul_f64(double a, double b          ) { ASM_BARRIER; return a * b; }
COMPUTATION_FN double do_fma_f64(double a, double b, double c) { ASM_BARRIER; return std::fma(a, b, c); }

COMPUTATION_FN float4 do_div_v4(float4 a, float4 b          ) { ASM_BARRIER; return a / b; }
COMPUTATION_FN float4 do_add_v4(float4 a, float4 b          ) { ASM_BARRIER; return a + b; }
COMPUTATION_FN float4 do_mul_v4(float4 a, float4 b          ) { ASM_BARRIER; return a * b; }
COMPUTATION_FN float4 do_fma_v4(float4 a, float4 b, float4 c) {
  /* No direct GCC vector FMA; loop over lanes to get llvm.fma.f32 per lane. */
  ASM_BARRIER;
  float ta[4], tb[4], tc[4], tr[4];
  memcpy(ta, &a, sizeof(a)); memcpy(tb, &b, sizeof(b)); memcpy(tc, &c, sizeof(c));
  for (int i = 0; i < 4; i++) tr[i] = std::fma(ta[i], tb[i], tc[i]);
  float4 r; memcpy(&r, tr, sizeof(r)); return r;
}

COMPUTATION_FN double2 do_div_v2d(double2 a, double2 b           ) { ASM_BARRIER; return a / b; }
COMPUTATION_FN double2 do_add_v2d(double2 a, double2 b           ) { ASM_BARRIER; return a + b; }
COMPUTATION_FN double2 do_mul_v2d(double2 a, double2 b           ) { ASM_BARRIER; return a * b; }
COMPUTATION_FN double2 do_fma_v2d(double2 a, double2 b, double2 c) {
  /* No direct GCC vector FMA; loop over lanes to get llvm.fma.f64 per lane. */
  ASM_BARRIER;
  double ta[2], tb[2], tc[2], tr[2];
  memcpy(ta, &a, sizeof(a)); memcpy(tb, &b, sizeof(b)); memcpy(tc, &c, sizeof(c));
  for (int i = 0; i < 2; i++) tr[i] = std::fma(ta[i], tb[i], tc[i]);
  double2 r; memcpy(&r, tr, sizeof(r)); return r;
}

/* ---- Op enum and dispatch ----------------------------------------------- */

enum class Op { DIV, ADD, MUL, FMA };

static Op parse_op(const char *s) {
  if (std::string(s) == "div") return Op::DIV;
  if (std::string(s) == "add") return Op::ADD;
  if (std::string(s) == "mul") return Op::MUL;
  if (std::string(s) == "fma") return Op::FMA;
  fprintf(stderr, "Unknown op: %s\n", s);
  exit(1);
}

static const char *op_name(Op op) {
  switch (op) {
  case Op::DIV: return "div";
  case Op::ADD: return "add";
  case Op::MUL: return "mul";
  case Op::FMA: return "fma";
  }
  return "?";
}

template <typename T> static T run_op(Op op, T a, T b, T c);
template <> float  run_op(Op op, float  a, float  b, float  c) {
  switch (op) {
  case Op::DIV: return do_div_f32(a, b);
  case Op::ADD: return do_add_f32(a, b);
  case Op::MUL: return do_mul_f32(a, b);
  case Op::FMA: return do_fma_f32(a, b, c);
  }
  abort();
}
template <> double run_op(Op op, double a, double b, double c) {
  switch (op) {
  case Op::DIV: return do_div_f64(a, b);
  case Op::ADD: return do_add_f64(a, b);
  case Op::MUL: return do_mul_f64(a, b);
  case Op::FMA: return do_fma_f64(a, b, c);
  }
  abort();
}
static float4 run_op_v4(Op op, float4 a, float4 b, float4 c) {
  switch (op) {
  case Op::DIV: return do_div_v4(a, b);
  case Op::ADD: return do_add_v4(a, b);
  case Op::MUL: return do_mul_v4(a, b);
  case Op::FMA: return do_fma_v4(a, b, c);
  }
  abort();
}
static double2 run_op_v2d(Op op, double2 a, double2 b, double2 c) {
  switch (op) {
  case Op::DIV: return do_div_v2d(a, b);
  case Op::ADD: return do_add_v2d(a, b);
  case Op::MUL: return do_mul_v2d(a, b);
  case Op::FMA: return do_fma_v2d(a, b, c);
  }
  abort();
}

/* ---- fp_traits ---------------------------------------------------------- */

template <typename T> struct fp_traits;
template <> struct fp_traits<float>  {
  using uint_t = uint32_t;
  static constexpr int total_bits = 24; /* 1 implicit + 23 explicit mantissa */
};
template <> struct fp_traits<double> {
  using uint_t = uint64_t;
  static constexpr int total_bits = 53; /* 1 implicit + 52 explicit mantissa */
};

template <typename T>
static auto to_bits(T x) -> typename fp_traits<T>::uint_t {
  typename fp_traits<T>::uint_t raw{};
  memcpy(&raw, &x, sizeof(T));
  return raw;
}

/* ---- Exact reference in long double (not instrumented by PRISM) --------- */

template <typename T>
static long double exact_ref(Op op, T a, T b, T c) {
  const long double la = a, lb = b, lc = c;
  switch (op) {
  case Op::DIV: return la / lb;
  case Op::ADD: return la + lb;
  case Op::MUL: return la * lb;
  case Op::FMA: return la * lb + lc; /* exact in long double arithmetic */
  }
  abort();
}

/* ---- Scalar check ------------------------------------------------------- */

/*
 * At precision p, SR rounds the exact result to one of two consecutive
 * "p-bit floats" (lo, hi) that differ by exactly 2^noise_bits in the integer
 * representation (= 1 ULP at precision p).
 *
 * Note: lo and hi can differ at more than one bit when carry propagates
 * (e.g. lo = ...0101 0101 and hi = lo+1 = ...0101 0110 differ at bits 0 and
 * 1).  We verify brackets by checking the integer difference, not bit ranges.
 */
template <typename T>
static int check_scalar(const char *label, Op op, T a, T b, T c, int p) {
  using uint_t = typename fp_traits<T>::uint_t;
  constexpr int total_bits = fp_traits<T>::total_bits;
  const int noise_bits = total_bits - p;

  const uint_t expected_diff = uint_t{1} << noise_bits;

  uint_t patterns[N];
  long double sum = 0.0L;

  for (int i = 0; i < N; i++) {
    const T s = run_op(op, a, b, c); /* PRISM-instrumented */
    patterns[i] = to_bits(s);
    sum += static_cast<long double>(s); /* fpext: not instrumented */
  }

  /* BRACKETS: exactly 2 distinct values, differing by 2^noise_bits. */
  uint_t lo = patterns[0], hi = patterns[0];
  bool two_values = false;
  for (int i = 1; i < N; i++) {
    if (patterns[i] == lo || patterns[i] == hi)
      continue;
    if (!two_values) {
      hi = patterns[i];
      two_values = true;
      if (hi < lo) { uint_t t = lo; lo = hi; hi = t; }
    } else {
      printf("FAIL %s BRACKETS: 3rd distinct value 0x%llx at sample %d"
             " (expected only 0x%llx and 0x%llx)\n",
             label, (unsigned long long)patterns[i], i,
             (unsigned long long)lo, (unsigned long long)hi);
      return 1;
    }
  }
  if (two_values && hi - lo != expected_diff) {
    printf("FAIL %s BRACKET_SPACING: diff=0x%llx, expected 0x%llx (2^%d)\n",
           label,
           (unsigned long long)(hi - lo),
           (unsigned long long)expected_diff,
           noise_bits);
    return 1;
  }

  /* STOCHASTIC: both brackets must appear. */
  if (!two_values) {
    printf("FAIL %s STOCHASTIC: only 1 distinct value in %d samples (p=%d)\n",
           label, N, p);
    return 1;
  }

  /* UNBIASED: E[SR_p(op(a,b,c))] = exact result.
   * Long double ref and mean avoid IEEE summation error at high precisions. */
  T lo_val, hi_val;
  memcpy(&lo_val, &lo, sizeof(T));
  memcpy(&hi_val, &hi, sizeof(T));
  const long double ref_ld  = exact_ref(op, a, b, c);
  const long double mean_ld = sum / static_cast<long double>(N);
  const long double ulp_ld  = static_cast<long double>(hi_val) - static_cast<long double>(lo_val);
  const long double allowed = 5.0L * 0.5L * ulp_ld / sqrtl(static_cast<long double>(N));
  const long double err     = fabsl(mean_ld - ref_ld);
  if (err > allowed) {
    printf("FAIL %s UNBIASED: mean_err=%.3Le > 5sigma=%.3Le (p=%d)\n",
           label, err, allowed, p);
    return 1;
  }

  printf("PASS %-34s  p=%2d  noise_bits=%2d  lo=0x%llx hi=0x%llx\n",
         label, p, noise_bits,
         (unsigned long long)lo, (unsigned long long)hi);
  return 0;
}

/* ---- Vector check ------------------------------------------------------- */

template <typename T, typename Vec, int LANES>
static int check_vector(const char *label, Op op, T a, T b, T c, int p,
                        Vec (*run_vec_op)(Op, Vec, Vec, Vec)) {
  using uint_t = typename fp_traits<T>::uint_t;
  constexpr int total_bits = fp_traits<T>::total_bits;
  const int noise_bits = total_bits - p;

  const uint_t expected_diff = uint_t{1} << noise_bits;

  T a_lanes[LANES], b_lanes[LANES], c_lanes[LANES];
  for (int lane = 0; lane < LANES; lane++) {
    a_lanes[lane] = a;
    b_lanes[lane] = b;
    c_lanes[lane] = c;
  }

  Vec va, vb, vc;
  memcpy(&va, a_lanes, sizeof(va));
  memcpy(&vb, b_lanes, sizeof(vb));
  memcpy(&vc, c_lanes, sizeof(vc));

  uint_t lane_lo[LANES]{}, lane_hi[LANES]{};
  uint_t patterns[N][LANES]{};
  bool   lane_two[LANES]{};
  bool   init_done = false;

  for (int i = 0; i < N; i++) {
    const Vec res = run_vec_op(op, va, vb, vc); /* PRISM-instrumented */
    T tmp[LANES];
    memcpy(tmp, &res, sizeof(res));
    uint_t bits[LANES];
    for (int lane = 0; lane < LANES; lane++)
      memcpy(&bits[lane], &tmp[lane], sizeof(T));
    memcpy(patterns[i], bits, sizeof(bits));

    for (int lane = 0; lane < LANES; lane++) {
      if (!init_done) {
        lane_lo[lane] = lane_hi[lane] = bits[lane];
      } else {
        if (bits[lane] != lane_lo[lane] && bits[lane] != lane_hi[lane]) {
          if (!lane_two[lane]) {
            lane_hi[lane] = bits[lane];
            lane_two[lane] = true;
            if (lane_hi[lane] < lane_lo[lane]) {
              uint_t t = lane_lo[lane]; lane_lo[lane] = lane_hi[lane]; lane_hi[lane] = t;
            }
          } else {
            printf("FAIL %s BRACKETS: lane %d 3rd distinct value 0x%llx at sample %d\n",
                   label, lane, (unsigned long long)bits[lane], i);
            return 1;
          }
        }
      }
    }
    init_done = true;
  }

  for (int lane = 0; lane < LANES; lane++) {
    if (!lane_two[lane]) {
      printf("FAIL %s STOCHASTIC: lane %d only 1 distinct value (p=%d)\n",
             label, lane, p);
      return 1;
    }
    if (lane_hi[lane] - lane_lo[lane] != expected_diff) {
      printf("FAIL %s BRACKET_SPACING: lane %d diff=0x%llx expected 0x%llx\n",
             label, lane,
             (unsigned long long)(lane_hi[lane] - lane_lo[lane]),
             (unsigned long long)expected_diff);
      return 1;
    }
  }

  int lane_hi_count[LANES]{};
  int all_same_count = 0;
  for (int i = 0; i < N; i++) {
    bool all_same = true;
    for (int lane = 0; lane < LANES; lane++) {
      if (patterns[i][lane] == lane_hi[lane])
        lane_hi_count[lane]++;
      if (lane > 0 && patterns[i][lane] != patterns[i][0])
        all_same = false;
    }
    if (all_same)
      all_same_count++;
  }

  /* INDEPENDENCE: compare all-same observations with the rate expected from
   * each lane's observed bracket frequencies. Skewed two-lane cases can
   * legitimately exceed 90%, so a fixed threshold is too strict. */
  long double prob_all_hi = 1.0L;
  long double prob_all_lo = 1.0L;
  for (int lane = 0; lane < LANES; lane++) {
    const long double q = static_cast<long double>(lane_hi_count[lane]) /
                          static_cast<long double>(N);
    prob_all_hi *= q;
    prob_all_lo *= (1.0L - q);
  }
  const long double prob_all_same = prob_all_hi + prob_all_lo;
  const long double expected = static_cast<long double>(N) * prob_all_same;
  const long double sigma = sqrtl(static_cast<long double>(N) *
                                  prob_all_same *
                                  (1.0L - prob_all_same));
  if (static_cast<long double>(all_same_count) > expected + 5.0L * sigma) {
    printf("FAIL %s INDEPENDENCE: all lanes rounded same in %d/%d runs"
           " (expected %.1Lf + 5sigma %.1Lf)\n",
           label, all_same_count, N, expected, 5.0L * sigma);
    return 1;
  }

  printf("PASS %-34s  p=%2d  noise_bits=%2d  all_same=%d/%d\n",
         label, p, noise_bits, all_same_count, N);
  return 0;
}

/* ---- main -------------------------------------------------------------- */

int main(int argc, char **argv) {
  if (argc < 6) {
    fprintf(stderr,
            "Usage: %s <float|double|vector|vector-double> <div|add|mul|fma>"
            " <precision> <a> <b> [<c>]\n",
            argv[0]);
    return 1;
  }

  const std::string mode = argv[1];
  const Op    op = parse_op(argv[2]);
  const int    p = atoi(argv[3]);
  const double a = strtod(argv[4], nullptr);
  const double b = strtod(argv[5], nullptr);
  const double c = (argc > 6) ? strtod(argv[6], nullptr) : 0.0;

  char label[64];
  snprintf(label, sizeof(label), "%s_%s", mode.c_str(), op_name(op));

  int rc = 0;

  if (mode == "float") {
    rc |= check_scalar<float>(label, op,
                              static_cast<float>(a),
                              static_cast<float>(b),
                              static_cast<float>(c), p);
  } else if (mode == "double") {
    rc |= check_scalar<double>(label, op, a, b, c, p);
  } else if (mode == "vector") {
    rc |= check_vector<float, float4, 4>(label, op,
                                         static_cast<float>(a),
                                         static_cast<float>(b),
                                         static_cast<float>(c), p,
                                         run_op_v4);
  } else if (mode == "vector-double") {
    rc |= check_vector<double, double2, 2>(label, op, a, b, c, p,
                                           run_op_v2d);
  } else {
    fprintf(stderr, "Unknown mode: %s\n", mode.c_str());
    return 1;
  }

  return rc;
}
