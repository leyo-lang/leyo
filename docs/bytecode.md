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

## Opcode Groups

The existing opcodes are grouped roughly as follows:

- register setup: `OP_PUT_A`, `OP_PUT_B`, `OP_PUT_S`, `OP_PUT_A_R`, `OP_PUT_B_R`
- arithmetic: `OP_OPERATE_MUL`, `OP_OPERATE_DIV`, `OP_OPERATE_ADD`, `OP_OPERATE_SUB`, `OP_OPERATE_EXP`
- globals: `OP_STORE`, `OP_LOAD`
- constants: `OP_CONST_LOAD`
- stack helpers: `OP_SS_PUSH_A`, `OP_SS_PUSH_B`, `OP_SS_POP`
- terminator: `OP_FINISH`

## Notes For Contributors

- Keep opcode values stable unless you are intentionally breaking compatibility
- Update the disassembler when adding or changing opcodes
- Update the VM and parser in the same change when you alter instruction shapes

For the quick human-readable opcode list, see [`docs/opcodes.txt`](opcodes.txt).
