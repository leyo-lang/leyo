# Virtual Machine

The Leyo VM executes the bytecode emitted by the compiler. Its implementation lives in [`src/vm.c`](../src/vm.c).

## Runtime Model

The VM keeps a small register set:

- `A`
- `B`
- `R`

It also maintains:

- a global variable table
- a constant pool decoded from the `.lybc` file
- an instruction pointer

## Instruction Flow

The VM reads bytes sequentially and dispatches on opcodes defined in [`include/bytecode.h`](../include/bytecode.h).

Notable instructions:

- `OP_PUT_A` loads an immediate into `A`
- `OP_PUT_B` loads an immediate into `B`
- `OP_PUT_S` swaps `A` and `B`
- `OP_PUT_A_R` copies `R` into `A`
- `OP_PUT_B_R` copies `R` into `B`
- arithmetic opcodes compute into `R`
- `OP_STORE` writes a value into a global slot
- `OP_LOAD` reads a global slot into `R`
- `OP_CONST_LOAD` loads a value from the constant pool into `R`
- `OP_FINISH` ends execution

## Constant Pool

The parser serializes constants separately from the instruction stream. The VM decodes them before execution starts.

This is important for contributors:

- do not change the constant encoding without updating both compiler and VM
- do not change the opcode operand width without updating the decoder

## Debugging Tips

- Check `logs/latest.lylog` first when a program fails
- If bytecode looks wrong, disassemble the file before changing the VM
- If the VM sees the wrong opcode, verify the parser emitted the expected operand bytes

## Current Limitations

- The VM is intentionally small
- The supported instruction set is limited to the current compiler output
- New opcodes should be added together with a disassembler update
