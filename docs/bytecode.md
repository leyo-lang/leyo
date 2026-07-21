# Bytecode Format

Leyo bytecode is defined by the constants in [`include/bytecode.h`](../include/bytecode.h) and the file wrapper in [`include/headerer.h`](../include/headerer.h).

## File Layout

Current `.lybc` files are laid out as:

1. Leyo header
2. Instruction stream
3. Serialized constant pool

## Header

The header stores:

- magic bytes
- version string
- flags
- code size

The runtime uses the header to validate the file before execution.

## Notes For Contributors

- Keep opcode values stable unless you are intentionally breaking compatibility
- Update the disassembler when adding or changing opcodes
- Update the VM and parser in the same change when you alter instruction shapes

For the full opcode list, see [`include/bytecode.h`](/include/bytecode.h).
