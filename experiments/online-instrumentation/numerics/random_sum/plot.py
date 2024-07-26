import numpy as np
import plotly.express as px
import pandas as pd
import glob
import argparse
import joblib
from functools import reduce


def parse_file(file):
    with open(file, "r") as f:
        for line in f:
            if "msec" in line:
                return float(line.split()[0])


def run():
    files = glob.glob("*.perf")
    values = {}
    for file in files:
        # opt_mode_type.perf
        opt, mode, type = file.split(".")[0].split("_")
        x = parse_file(file)
        values["opt"] = values.get("opt", []) + [opt]
        values["mode"] = values.get("mode", []) + [mode]
        values["type"] = values.get("type", []) + [type]
        values["slowdown"] = values.get("slowdown", []) + [x]
    return pd.DataFrame(values)


def main():
    df = run()
    df[df["opt"] == "O0"]["slowdown"] /= 2  # O0
    df[df["opt"] == "O0"]["slowdown"] /= 0.7  # O3
    print(df)
    fig = px.bar(
        df, x="opt", y="slowdown", color="mode", barmode="group", facet_col="type"
    )
    # fig.update_yaxes(title_text="Slowdown")
    fig.update_layout(
        font=dict(
            size=22,
        )
    )
    fig.show()


if __name__ == "__main__":
    main()


# /usr/local/bin/verificarlo-c++ -DAT_PER_OPERATOR_HEADERS -DCAFFE2_BUILD_MAIN_LIB -DCPUINFO_SUPPORTED_PLATFORM=1 -DFMT_HEADER_ONLY=1 -DFXDIV_USE_INLINE_ASSEMBLY=0 -DHAVE_MALLOC_USABLE_SIZE=1 -DHAVE_MMAP=1 -DHAVE_SHM_OPEN=1 -DHAVE_SHM_UNLINK=1 -DMINIZ_DISABLE_ZIP_READER_CRC32_CHECKS -DNNP_CONVOLUTION_ONLY=0 -DNNP_INFERENCE_ONLY=0 -DONNXIFI_ENABLE_EXT=1 -DONNX_ML=1 -DONNX_NAMESPACE=onnx_torch -DUSE_C10D_GLOO -DUSE_DISTRIBUTED -DUSE_EXTERNAL_MZCRC -DUSE_RPC -DUSE_TENSORPIPE -D_FILE_OFFSET_BITS=64 -Dtorch_cpu_EXPORTS -I/var/lib/jenkins/workspace/third_party/pocketfft -I/var/lib/jenkins/workspace/build/aten/src -I/var/lib/jenkins/workspace/aten/src -I/var/lib/jenkins/workspace/build -I/var/lib/jenkins/workspace -I/var/lib/jenkins/workspace/cmake/../third_party/benchmark/include-I/var/lib/jenkins/workspace/third_party/onnx -I/var/lib/jenkins/workspace/build/third_party/onnx -I/var/lib/jenkins/workspace/third_party/foxi -I/var/lib/jenkins/workspace/build/third_party/foxi -I/var/lib/jenkins/workspace/torch/csrc/api -I/var/lib/jenkins/workspace/torch/csrc/api/include -I/var/lib/jenkins/workspace/caffe2/aten/src/TH -I/var/lib/jenkins/workspace/build/caffe2/aten/src/TH -I/var/lib/jenkins/workspace/build/caffe2/aten/src -I/var/lib/jenkins/workspace/build/caffe2/../aten/src -I/var/lib/jenkins/workspace/torch/csrc -I/var/lib/jenkins/workspace/third_party/miniz-2.1.0 -I/var/lib/jenkins/workspace/third_party/kineto/libkineto/include -I/var/lib/jenkins/workspace/third_party/kineto/libkineto/src -I/var/lib/jenkins/workspace/aten/src/ATen/.. -I/var/lib/jenkins/workspace/third_party/FXdiv/include -I/var/lib/jenkins/workspace/c10/.. -I/var/lib/jenkins/workspace/third_party/pthreadpool/include -I/var/lib/jenkins/workspace/third_party/cpuinfo/include -I/var/lib/jenkins/workspace/third_party/QNNPACK/include -I/var/lib/jenkins/workspace/aten/src/ATen/native/quantized/cpu/qnnpack/include -I/var/lib/jenkins/workspace/aten/src/ATen/native/quantized/cpu/qnnpack/src -I/var/lib/jenkins/workspace/third_party/cpuinfo/deps/clog/include -I/var/lib/jenkins/workspace/third_party/NNPACK/include -I/var/lib/jenkins/workspace/third_party/fbgemm/include -I/var/lib/jenkins/workspace/third_party/fbgemm -I/var/lib/jenkins/workspace/third_party/fbgemm/third_party/asmjit/src -I/var/lib/jenkins/workspace/third_party/ittapi/src/ittnotify -I/var/lib/jenkins/workspace/third_party/FP16/include -I/var/lib/jenkins/workspace/third_party/tensorpipe -I/var/lib/jenkins/workspace/build/third_party/tensorpipe -I/var/lib/jenkins/workspace/third_party/tensorpipe/third_party/libnop/include -I/var/lib/jenkins/workspace/third_party/fmt/include -I/var/lib/jenkins/workspace/third_party/flatbuffers/include -isystem /var/lib/jenkins/workspace/build/third_party/gloo -isystem /var/lib/jenkins/workspace/cmake/../third_party/gloo -isystem /var/lib/jenkins/workspace/cmake/../third_party/tensorpipe/third_party/libuv/include -isystem /var/lib/jenkins/workspace/cmake/../third_party/googletest/googlemock/include -isystem /var/lib/jenkins/workspace/cmake/../third_party/googletest/googletest/include -isystem /var/lib/jenkins/workspace/third_party/protobuf/src -isystem /var/lib/jenkins/workspace/third_party/gemmlowp -isystem /var/lib/jenkins/workspace/third_party/neon2sse -isystem /var/lib/jenkins/workspace/third_party/XNNPACK/include -isystem /var/lib/jenkins/workspace/third_party/ittapi/include -isystem /var/lib/jenkins/workspace/cmake/../third_party/eigen -isystem /var/lib/jenkins/workspace/build/include -march=native  --online-instrumentation=up-down --inst-fma -D_GLIBCXX_USE_CXX11_ABI=1 -fvisibility-inlines-hidden -DUSE_PTHREADPOOL -DNDEBUG -DUSE_KINETO -DLIBKINETO_NOCUPTI -DLIBKINETO_NOROCTRACER -DUSE_FBGEMM -DUSE_QNNPACK -DUSE_PYTORCH_QNNPACK -DUSE_XNNPACK -DSYMBOLICATE_MOBILE_DEBUG_HANDLE -O2 -fPIC -Wall -Wextra -Werror=return-type -Werror=non-virtual-dtor -Werror=braced-scalar-init -Wnarrowing -Wno-missing-field-initializers -Wno-type-limits -Wno-array-bounds -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-function -Wno-unused-result -Wno-strict-overflow -Wno-strict-aliasing -Wvla-extension -Wnewline-eof -Winconsistent-missing-override -Winconsistent-missing-destructor-override -Wno-range-loop-analysis -Wno-pass-failed -Wno-error=pedantic -Wno-error=old-style-cast -Wno-error=inconsistent-missing-override -Wno-error=inconsistent-missing-destructor-override -Wconstant-conversion -Wno-invalid-partial-specialization -Wno-unused-private-field -Wno-missing-braces -Wunused-lambda-capture -Qunused-arguments -fcolor-diagnostics -faligned-new -fno-math-errno -fno-trapping-math -Werror=format -DHAVE_AVX512_CPU_DEFINITION -DHAVE_AVX2_CPU_DEFINITION -O3 -DNDEBUG -DNDEBUG -std=gnu++17 -fPIC -DTORCH_USE_LIBUV -DCAFFE2_USE_GLOO -DTH_HAVE_THREAD -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-unused-result -Wno-missing-field-initializers -Wno-unknown-pragmas -Wno-type-limits -Wno-array-bounds -Wno-strict-overflow -Wno-strict-aliasing -Wno-missing-braces -Wno-range-loop-analysis -fvisibility=hidden -O2 -Wmissing-prototypes -Werror=missing-prototypes -pthread -DASMJIT_STATIC -fopenmp=libiomp5 -MD -MT caffe2/CMakeFiles/torch_cpu.dir/__/aten/src/ATen/native/mkl/SpectralOps.cpp.o -MF CMakeFiles/torch_cpu.dir/__/aten/src/ATen/native/mkl/SpectralOps.cpp.o.d -o CMakeFiles/torch_cpu.dir/__/aten/src/ATen/native/mkl/SpectralOps.cpp.o -c /var/lib/jenkins/workspace/aten/src/ATen/native/mkl/SpectralOps.cpp
