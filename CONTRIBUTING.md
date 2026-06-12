# Contributing to Leyo

Thanks for helping improve Leyo. This project is still evolving, so the most useful contributions are small, well-explained changes that keep the compiler, bytecode format, VM, and docs in sync.

## Before You Start

- Read the top-level [`README.md`](README.md)
- Skim the architecture docs in [`docs/`](docs)
- Check the current `logs/latest.lylog` if you are debugging compiler or VM behavior

## Build And Verify

```bash
make
./bin/leyo.exe build test.leyo
./bin/leyo.exe run a.lybc
./bin/leyo.exe disassemble a.lybc --head
```

On Windows, you can also run the smoke test script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

If you change the CLI or file format, verify both `build` and `run`. A parser-only change can still break the VM if the emitted bytecode changes.

## Where To Make Changes

- Lexer changes go in [`src/lexer.c`](src/lexer.c)
- Parser and emission changes go in [`src/parser.c`](src/parser.c)
- Header and file-format changes go in [`src/headerer.c`](src/headerer.c) and [`include/headerer.h`](include/headerer.h)
- VM changes go in [`src/vm.c`](src/vm.c)
- CLI changes go in [`src/main.c`](src/main.c)
- Opcode changes go in [`include/bytecode.h`](include/bytecode.h)

## Style Notes

- Prefer clear, explicit control flow over clever tricks
- Keep bytecode encoding and decoding mirrored between compiler and VM
- Avoid changing opcode values casually because that can break existing `.lybc` files
- Keep docs and code aligned in the same change when behavior changes
- Use ASCII unless there is a strong reason to introduce non-ASCII text

## Tests And Validation

There is not a full automated test suite yet, so use the sample flow to validate changes:

1. Build the project
2. Compile `test.leyo`
3. Run the generated `.lybc`
4. Disassemble the output if you changed bytecode layout

If you add new syntax or opcodes, include at least one minimal sample program in the repo or update the existing sample.

## Documentation Expectations

Any user-visible change should update at least one of:

- `README.md`
- `docs/cli.md`
- `docs/compiler.md`
- `docs/vm.md`
- `docs/bytecode.md`

If a change affects contributor workflow, update this file too.

## Ownership Standard

When we merge a change, it should feel like it was designed by the project, not bolted onto it:

- keep wording direct and specific
- prefer docs that explain how the repo actually behaves today
- avoid leaving stale examples or dead command references around
- update command-line docs, opcode docs, and contributor notes together when behavior changes

## Pull Request Checklist

- Build succeeds
- `build` and `run` both work on `test.leyo`
- Documentation matches the new behavior
- New warnings are explained or fixed
- Generated files are not committed unless they are intentionally part of the change

## Reporting Bugs

If you open an issue, include:

- Your OS
- The exact command you ran
- The source file or bytecode file used
- The log output from `logs/latest.lylog`
- A minimal reproduction if possible

That makes it much easier to reproduce parser and VM bugs quickly.
