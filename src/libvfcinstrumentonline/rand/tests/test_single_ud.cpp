#include <iostream>
#include <map>

#include "hwy/highway.h"
#include "hwy/print-inl.h"

#include "src/ud.h"

const auto op_map = std::map<std::string, int>{
    {"add", 0}, {"sub", 1}, {"mul", 2}, {"div", 3}, {"sqrt", 4}, {"fma", 5},
};
const auto type_map = std::map<std::string, int>{
    {"f32", 0},
    {"f64", 1},
};

template <typename T>
void test_op(const std::vector<T> &args, int op, int num_args) {

  constexpr const char *fptype = std::is_same_v<T, float> ? "f32" : "f64";
  constexpr const char *opname[] = {"Add", "Sub", "Mul", "Div", "Sqrt", "Fma"};
  constexpr const char *fmt = std::is_same_v<T, float> ? "%+.6a " : "%+.13a ";

  const std::string opstr = opname[op];
  std::cerr << opstr << fptype << ":\n";

  switch (op) {
  case 0:
    fprintf(stderr, fmt, ud::scalar::add(args[0], args[1]));
    break;
  case 1:
    fprintf(stderr, fmt, ud::scalar::sub(args[0], args[1]));
    break;
  case 2:
    fprintf(stderr, fmt, ud::scalar::mul(args[0], args[1]));
    break;
  case 3:
    fprintf(stderr, fmt, ud::scalar::div(args[0], args[1]));
    break;
  case 4:
    fprintf(stderr, fmt, ud::scalar::sqrt(args[0]));
    break;
  case 5:
    fprintf(stderr, fmt, ud::scalar::fma(args[0], args[1], args[2]));
    break;
  }

  std::cerr << std::endl;
}

int main(int argc, char *argv[]) {

  if (argc <= 2) {
    std::cerr << "Usage: " << argv[0] << " <type> <op> <args...>\n";
    return 1;
  }

  const std::string type = std::string(argv[1]);
  const std::string op = std::string(argv[2]);

  if ((op == "sqrt") and argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <type> sqrt <arg1>\n";
    return 1;
  } else if ((op == "fma") and argc != 6) {
    std::cerr << "Usage: " << argv[0] << " <type> fma <arg1> <arg2> <arg3>\n";
    return 1;
  } else if (((op == "add") or (op == "sub") or (op == "mul") or
              (op == "div")) and
             argc != 5) {
    std::cerr << "Usage: " << argv[0] << " <type> <op> <arg1> <arg2> \n";
    return 1;
  }

  std::vector<double> args(3);
  args[0] = strtod(argv[3], NULL);
  if (argc > 4)
    args[1] = strtod(argv[4], NULL);
  if (argc > 5)
    args[2] = strtod(argv[5], NULL);

  std::vector<float> argsf;
  for (const auto &arg : args)
    argsf.push_back(static_cast<float>(arg));

  if (type == "f32")
    test_op<float>(argsf, op_map.at(op), argsf.size());
  else if (type == "f64")
    test_op<double>(args, op_map.at(op), args.size());

  return 0;
}
