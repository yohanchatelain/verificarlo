#include <iostream>
#include <map>

#include "hwy/highway.h"
#include "hwy/print-inl.h"

#include "src/ud.h"

template <typename T> void test_bin(T a, T b, int op) {

  constexpr const char *fptype = std::is_same_v<T, float> ? "f32" : "f64";
  constexpr const char *opname[] = {"Add", "Sub", "Mul", "Div"};
  constexpr const char *fmt = std::is_same_v<T, float> ? "%+.6a " : "%+.13a ";

  const std::string opstr = opname[op];
  std::cerr << opstr << fptype << ":\n";

  switch (op) {
  case 0:
    fprintf(stderr, fmt, ud::scalar::add(a, b));
    break;
  case 1:
    fprintf(stderr, fmt, ud::scalar::sub(a, b));
    break;
  case 2:
    fprintf(stderr, fmt, ud::scalar::mul(a, b));
    break;
  case 3:
    fprintf(stderr, fmt, ud::scalar::div(a, b));
    break;
  }

  std::cerr << std::endl;
}

int main(int argc, char *argv[]) {

  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " <float> <float> <op> <type>\n";
    return 1;
  }

  // print HWY_TARGET
  std::cout << "HWY_TARGET: " << HWY_TARGET << std::endl;

  double a = std::stof(argv[1]);
  double b = std::stof(argv[2]);
  const std::string op = std::string(argv[3]);
  const std::string type = std::string(argv[4]);

  const auto op_map = std::map<std::string, int>{
      {"add", 0},
      {"sub", 1},
      {"mul", 2},
      {"div", 3},
  };

  const auto type_map = std::map<std::string, int>{
      {"f32", 0},
      {"f64", 1},
  };

  if (type == "f32") {
    test_bin<float>(a, b, op_map.at(op));
  } else if (type == "f64") {
    test_bin<double>(a, b, op_map.at(op));
  }

  return 0;
}
