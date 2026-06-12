# Command Line Reference

The CLI is defined in [`src/main.c`](../src/main.c). The current executable supports three primary commands:

## `build`

```bash
./bin/leyo.exe build <source> [output]
```

- Reads a `.leyo` source file
- Tokenizes the source
- Parses it into bytecode
- Wraps the output in a Leyo header
- Writes the result to the destination file

If `output` is omitted, the default destination is `a.lybc`.

Examples:

```bash
./bin/leyo.exe build test.leyo
./bin/leyo.exe build test.leyo custom.lybc
```

## `run`

```bash
./bin/leyo.exe run <file.lybc>
```

- Reads a `.lybc` file
- Validates the header
- Loads the bytecode section
- Loads the constant pool if present
- Executes the program in the VM

Example:

```bash
./bin/leyo.exe run a.lybc
```

## `disassemble`

```bash
./bin/leyo.exe disassemble <file.lybc> [--hex] [--head]
```

- Shows a human-readable opcode listing by default
- `--hex` prints raw opcode bytes
- `--head` skips the header before disassembly

Examples:

```bash
./bin/leyo.exe disassemble a.lybc
./bin/leyo.exe disassemble a.lybc --hex
./bin/leyo.exe disassemble a.lybc --head
```

## Notes

- The CLI currently does not implement a full help system
- Some older project notes mention commands that are not wired up in `main.c` yet
- When in doubt, trust the command behavior in code over the legacy docs in `docs/files.txt`

## Next CLI Additions

These are the first CLI additions I want in the project once the core flows stay stable:

- `help` for command and flag summaries
- `version` for build and release metadata
- `log <file>` to choose the active log file from the command line
- `repl` for interactive language experiments
- `test` to run built-in regression checks
- `check <source>` to validate source without writing output
- `fmt <source>` to normalize source formatting
- `run --trace` to show each VM step during execution
- `disassemble --json` for tooling and editor integrations

These are suggestions, not promises. I keep them here so future CLI work has a clear direction instead of growing ad hoc flags.
