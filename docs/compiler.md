# Compiler Pipeline

This document describes the current source-to-bytecode flow in the compiler side of the project.

## Pipeline Overview

1. [`src/main.c`](../src/main.c) loads source text from disk
2. [`src/lexer.c`](../src/lexer.c) converts source text into tokens
3. [`src/parser.c`](../src/parser.c) consumes tokens and emits bytecode
4. [`src/headerer.c`](../src/headerer.c) prepends the Leyo file header
5. [`src/main.c`](../src/main.c) writes the final `.lybc` file

## Lexer

The lexer turns raw characters into tokens defined in [`include/type.h`](../include/type.h).

Current token categories include:

- identifiers
- numbers
- strings
- chars
- operators
- semicolons
- braces and brackets
- end-of-stream

Comments use `~` as the delimiter in the current lexer implementation.

## Parser

The parser currently handles:

- variable declarations using the type names `int`, `str`, `flt`, and `chr`
- assignments to previously declared globals
- simple binary expressions using `+`, `-`, `*`, `/`, and `^`

Expression atoms currently include:

- numeric literals
- identifiers

The parser also tracks a constant pool so literals can be stored separately from the bytecode stream.

## Bytecode Emission

The compiler emits instruction bytes defined in [`include/bytecode.h`](../include/bytecode.h).

The current flow uses:

- `OP_CONST_LOAD` for literal loading
- `OP_PUT_A`, `OP_PUT_B`, and `OP_PUT_S` for register setup
- arithmetic opcodes for expression evaluation
- `OP_STORE` for global assignment
- `OP_FINISH` to mark the end of execution

## Output Format

The `.lybc` file currently contains:

- a fixed Leyo header
- the instruction stream
- the serialized constant pool

That means compiler and VM changes must stay in sync. If you change the emitted layout, update the VM loader at the same time.

## Current Limitations

- The language is still small and incomplete
- String and char literals are lexed, but expression support is limited
- The parser and VM should always be changed together when bytecode semantics change
