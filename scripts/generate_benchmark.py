# generate_benchmark.py
import sys

COUNT = int(sys.argv[1])

with open("tests/benchmark.leyo", "w") as f:
    f.write("// Leyo VM stress benchmark\n\n")
    f.write("fnc main() <int> {\n")

    # Variable declarations
    for i in range(COUNT):
        f.write(f"int value{i} = {i};\n")

    f.write("\n")

    # Arithmetic operations
    for i in range(COUNT):
        f.write(
            f"value{i} = value{i} + {i};\n"
            f"value{i} = value{i} * 2;\n"
            f"value{i} = value{i} - 1;\n"
        )

    f.write("\n")

    # More expression evaluation
    for i in range(COUNT):
        f.write(
            f"int calc{i} = {i} + {i+1} * {i+2} - {i+3};\n"
        )

    f.write("\nrtn 0; };\n")

print(f"Generated {COUNT} variables and expressions")