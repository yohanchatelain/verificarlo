import argparse
import os
import re
import sys
import logging


logger = logging.getLogger("pprint")


class FloatType:
    def __init__(self, name, mantissa, precision, exponent):
        self.name = name
        self.mantissa = mantissa
        self.precision = precision
        self.exponent = exponent


Binary16 = FloatType("binary16", 10, 11, 5)
Binary32 = FloatType("binary32", 23, 24, 8)
Binary64 = FloatType("binary64", 52, 53, 11)
# Binary128 = FloatType("binary128", 112, 113, 15)
# Binary256 = FloatType("binary256", 236, 237, 19)

BFloat16 = FloatType("bfloat16", 7, 8, 8)
TFloat32 = FloatType("tf32", 10, 11, 8)


__fp_types = {Binary16, Binary32, Binary64, BFloat16, TFloat32}
__fp_types_names = {x.name: x for x in __fp_types}


def parse_number(x):
    try:
        return float.fromhex(x)
    except ValueError:
        print(f"Failed to parse {x} as a float")
        sys.exit(1)


def get_exponents(numbers: list[float]) -> dict[str, int]:
    # get last number after 'p'
    exponents = {x.hex(): int(x.hex().split("p")[-1]) for x in numbers}
    return exponents


def safe_search(pattern, string, group=0):
    match = re.search(pattern, string)
    if match is None:
        raise ValueError(f"Pattern {pattern} not found in {string}")
    logger.debug(f"{string} -> {match.group(group)}")
    return match.group(group)


def to_binary(num: str, type: FloatType) -> str:
    # convert float to binary
    # extract mantissa (hexadecimal) and convert to binary
    # number as format: [+-]?1.(mantissa)p[+-]?(exponent)
    logger.debug(f"Convert to binary: {num}")

    binary = ""
    sign_leading_bit = safe_search(r"([+-]?[01]\.)", num)
    mantissa = safe_search(r"0x[+-]?[01]\.(.*)p", num, group=1)

    for i in range(min(len(mantissa), type.precision // 4)):
        c = mantissa[i]
        binary += f"{bin(int(c,16))[2:]:>04s}"

    sign_leading_bit = sign_leading_bit.replace(".", "")
    binary = sign_leading_bit + binary

    binary = binary[: type.mantissa + 1]

    logger.debug(f"Binary: {binary}")

    return binary


def get_headers(numbers: dict[str, int], ftype: FloatType, grid_stride: int):
    max_exp = max(numbers.values())
    min_exp = min(numbers.values())
    # sort numbers by exponent
    numbers = dict(sorted(numbers.items(), key=lambda x: x[1], reverse=True))

    min_grid_exp = min_exp - ftype.precision - 1
    header = "".join(
        [f"{i:<{grid_stride}}" for i in range(max_exp, min_grid_exp, -grid_stride)]
    )
    header2 = "".join(
        [f"|{' '*(grid_stride-1)}" for i in range(max_exp, min_grid_exp, -grid_stride)]
    )
    return [header, header2]


def print_aligned(
    numbers: dict[str, int],
    ftype: FloatType,
    grid_stride: int,
    sort: bool = False,
):
    # print number aligned with their exponents
    max_exp = max(numbers.values())

    if sort:
        numbers = dict(sorted(numbers.items(), key=lambda x: x[1], reverse=True))

    [header, header2] = get_headers(numbers, ftype, grid_stride)

    print(header)
    print(header2)

    for num, exp in numbers.items():
        logger.debug(f"Number: {num}, Exponent: {exp}")
        shift = max_exp - exp
        logger.debug(f"Shift: {shift}")
        binary = to_binary(num, ftype)
        print(header2[:shift] + binary + header2[shift + len(binary) :])


def read_numbers(filename: str) -> list[str]:
    if not os.path.exists(filename):
        print(f"File {filename} does not exist")
        sys.exit(1)

    with open(filename, "r") as f:
        numbers = f.readlines()
    return numbers


def parse_args():
    parser = argparse.ArgumentParser(description="Run the experiment")
    parser.add_argument(
        "--type",
        type=str,
        choices=__fp_types_names.keys(),
        required=True,
        help="Type of the experiment",
    )
    parser.add_argument("--filename", type=str, help="File to read numbers from")
    parser.add_argument(
        "numbers",
        metavar="numbers",
        nargs="*",
        type=str,
        help="list of numbers",
    )
    parser.add_argument("--grid-stride", type=int, default=4, help="Grid stride")
    parser.add_argument("--sort", action="store_true", help="Sort numbers by exponent")
    parser.add_argument("--verbose", action="store_true", help="Print verbose output")
    return parser.parse_args()


def main():
    args = parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    numbers = read_numbers(args.filename) if args.filename else args.numbers
    if not numbers:
        print("No numbers to process")
        sys.exit(1)

    exponents = list(map(parse_number, numbers))
    exponents = get_exponents(exponents)
    type = __fp_types_names[args.type]
    print_aligned(exponents, type, grid_stride=args.grid_stride)


if __name__ == "__main__":
    main()
