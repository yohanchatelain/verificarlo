#define PAD_SIZE 4

struct __attribute__((packed)) veritracer_probe_binary32_fmt_t {
  uint32_t sizeof_value;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  float value;
  uint8_t pad[PAD_SIZE];
};

struct __attribute__((packed)) veritracer_probe_binary64_fmt_t {
  uint32_t sizeof_value;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  double value;
};

struct __attribute__((packed)) veritracer_probe_int32_fmt_t {
  uint32_t sizeof_value;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  int32_t value;
  uint8_t pad[PAD_SIZE];
};

struct __attribute__((packed)) veritracer_probe_int64_fmt_t {
  uint32_t sizeof_value;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  int64_t value;
};

int sizeof_binary32_fmt = sizeof(struct veritracer_probe_binary32_fmt_t);
int sizeof_binary64_fmt = sizeof(struct veritracer_probe_binary64_fmt_t);
int sizeof_int32_fmt = sizeof(struct veritracer_probe_int32_fmt_t);
int sizeof_int64_fmt = sizeof(struct veritracer_probe_int64_fmt_t);

static uint64_t get_timestamp(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t usecs = tv.tv_sec * 1000000ull + tv.tv_usec;
  return usecs;
}

/* Probes used by the veritracer pass */

/* Probes used for the text format */

/* int */

void _veritracer_probe_int32(int32_t value, int32_t *value_ptr,
                             uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "int32 %lu %lu %p %d\n", get_timestamp(), hash_LI,
          value_ptr, value);
}

void _veritracer_probe_int64(int64_t value, int64_t *value_ptr,
                             uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "int64 %lu %lu %p %ld\n", get_timestamp(), hash_LI,
          value_ptr, value);
}

/* binary32 */

void _veritracer_probe_binary32(float value, float *value_ptr,
                                uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", get_timestamp(),
          hash_LI, value_ptr, value);
}

void _veritracer_probe_binary32_ptr(float *value, float *value_ptr,
                                    uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", get_timestamp(),
          hash_LI, value_ptr, *value);
}

void _veritracer_probe_2xbinary32(float2 value, float *value_ptr,
                                  uint64_t hash_LI[2]) {
  const int N = 2;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", timestamp,
            hash_LI[i], value_ptr, value[i]);
}

void _veritracer_probe_4xbinary32(float4 value, float4 *value_ptr,
                                  uint64_t hash_LI[4]) {
  const int N = 4;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", timestamp,
            hash_LI[i], value_ptr, value[i]);
}

/* binary64 */

void _veritracer_probe_binary64(double value, double *value_ptr,
                                uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", get_timestamp(),
          hash_LI, value_ptr, value);
}

void _veritracer_probe_binary64_ptr(double *value, double *value_ptr,
                                    uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", get_timestamp(),
          hash_LI, value_ptr, *value);
}

void _veritracer_probe_2xbinary64(double2 value, double *value_ptr,
                                  uint64_t hash_LI[2]) {
  const int N = 2;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", timestamp,
            hash_LI[i], value_ptr, value[i]);
}

void _veritracer_probe_4xbinary64(double4 value, double *value_ptr,
                                  uint64_t hash_LI[4]) {
  const int N = 4;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", timestamp,
            hash_LI[i], value_ptr, value[i]);
}

/* Probes used for the binary format */

/* int */

void _veritracer_probe_int32_binary(int32_t value, int32_t *value_ptr,
                                    uint64_t hash_LI) {
  struct veritracer_probe_int32_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;
  fwrite(&fmt, sizeof_int32_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_int64_binary(int64_t value, int64_t *value_ptr,
                                    uint64_t hash_LI) {
  struct veritracer_probe_int64_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;
  fwrite(&fmt, sizeof_int64_fmt, 1, trace_FILE_ptr);
}

/* binary32 */

void _veritracer_probe_binary32_binary(float value, float *value_ptr,
                                       uint64_t hash_LI) {
  struct veritracer_probe_binary32_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;
  fwrite(&fmt, sizeof_binary32_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_binary32_binary_ptr(float *value, float *value_ptr,
                                           uint64_t hash_LI) {
  struct veritracer_probe_binary32_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = *value;
  fwrite(&fmt, sizeof_binary32_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_2xbinary32_binary(float2 value, float *value_ptr,
                                         uint64_t hash_LI[2]) {
  const int N = 2;
  struct veritracer_probe_binary32_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(float);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary32_fmt, N, trace_FILE_ptr);
}

void _veritracer_probe_4xbinary32_binary(float4 value, float *value_ptr,
                                         uint64_t hash_LI[4]) {
  const int N = 4;
  struct veritracer_probe_binary32_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(float);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary32_fmt, N, trace_FILE_ptr);
}

/* binary64 */

void _veritracer_probe_binary64_binary(double value, double *value_ptr,
                                       uint64_t hash_LI) {
  struct veritracer_probe_binary64_fmt_t fmt;
  fmt.sizeof_value = sizeof(double);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;
  fwrite(&fmt, sizeof_binary64_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_binary64_binary_ptr(double *value, double *value_ptr,
                                           uint64_t hash_LI) {
  struct veritracer_probe_binary64_fmt_t fmt;
  fmt.sizeof_value = sizeof(double);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = *value;
  fwrite(&fmt, sizeof_binary64_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_2xbinary64_binary(double2 value, double *value_ptr,
                                         uint64_t hash_LI[2]) {
  const int N = 2;
  struct veritracer_probe_binary64_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(double);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary64_fmt, N, trace_FILE_ptr);
}

void _veritracer_probe_4xbinary64_binary(double4 value, double *value_ptr,
                                         uint64_t hash_LI[4]) {
  const int N = 4;
  struct veritracer_probe_binary64_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(double);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary64_fmt, N, trace_FILE_ptr);
}

/* backtrace */

void get_backtrace(uint64_t hash_LI) {
  int nptrs = backtrace(backtrace_buffer, SIZE_MAX_BACKTRACE);
  ssize_t count = 20 + sizeof(backtrace_separator) - 1;
  backtrace_symbols_fd(backtrace_buffer, nptrs, backtrace_fd);
  sprintf(string_buffer, "%020lu%s", hash_LI, backtrace_separator);
  write(backtrace_fd, string_buffer, count);
}

void get_backtrace_x2(uint64_t hash_LI[2]) {
  int nptrs = backtrace(backtrace_buffer, SIZE_MAX_BACKTRACE);
  ssize_t count = 20 + 1 + 20 + sizeof(backtrace_separator) - 1;
  backtrace_symbols_fd(backtrace_buffer, nptrs, backtrace_fd);
  sprintf(string_buffer, "%020lu.%020lu%s", hash_LI[0], hash_LI[1],
          backtrace_separator);
  write(backtrace_fd, string_buffer, count);
}

void get_backtrace_x4(uint64_t hash_LI[4]) {
  int nptrs = backtrace(backtrace_buffer, SIZE_MAX_BACKTRACE);
  ssize_t count =
      20 + 1 + 20 + 1 + 20 + 1 + 20 + sizeof(backtrace_separator) - 1;
  backtrace_symbols_fd(backtrace_buffer, nptrs, backtrace_fd);
  sprintf(string_buffer, "%020lu.%020lu.%020lu.%020lu%s", hash_LI[0],
          hash_LI[1], hash_LI[2], hash_LI[3], backtrace_separator);
  write(backtrace_fd, string_buffer, sizeof(string_buffer) - 1);
}

int file_exist(const char *filename) {
  struct stat buffer;
  return stat(trace_filename, &buffer) == 0;
}

/* vfc tracer functions */
void vfc_tracer_init(void) {
  char *mode = NULL;
  mode = (file_exist(trace_filename)) ? "ab" : "wb";
  trace_FILE_ptr = fopen(trace_filename, mode);
  if (trace_FILE_ptr == NULL)
    errx(EXIT_FAILURE, "Could not open %s : %s\n", trace_filename,
         strerror(errno));

  mode = (file_exist(backtrace_filename)) ? "a" : "w";
  backtrace_FILE_ptr = fopen(backtrace_filename, mode);
  if (backtrace_FILE_ptr == NULL)
    errx(EXIT_FAILURE, "Could not open %s : %s\n", backtrace_filename,
         strerror(errno));

  backtrace_fd = fileno(backtrace_FILE_ptr);
  if (backtrace_fd == -1)
    errx(EXIT_FAILURE, "Could not open %s : %s\n", backtrace_filename,
         strerror(errno));
}

void vfc_tracer_exit(void) {
  if (trace_FILE_ptr != NULL) {
    fflush(trace_FILE_ptr);
    fclose(trace_FILE_ptr);
  }
  if (backtrace_FILE_ptr != NULL) {
    fflush(backtrace_FILE_ptr);
    fclose(backtrace_FILE_ptr);
  }
}
