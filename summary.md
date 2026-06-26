# Leyo Project Summary

## 1. Project Overview

### 1.0 What Leyo Is
Leyo is a small C-based language toolchain that turns source text into bytecode, stores that bytecode in a file format with a Leyo header, and then runs it in a virtual machine. The repository is intentionally compact, which means most changes touch several modules at once. That is a feature, not a bug: compiler, bytecode, loader, and VM logic stay close enough to understand together.

### 1.1 The Main Building Blocks
The repository is organized around a few core layers:

- `src/lexer.c` converts source text into tokens
- `src/parser.c` turns tokens into bytecode plus a constant pool
- `src/headerer.c` wraps the bytecode in a `.lybc` file header
- `src/runner.c` reads `.lybc` files and starts the VM
- `src/vm.c` executes the bytecode
- `src/disassembler.c` prints bytecode in human-readable or hex form
- `src/main.c` is the CLI dispatcher that connects everything
- `src/helper.c` prints the command help text that users see from the CLI
- `src/errors.c` owns logging and fatal error handling
- `src/lyst.c` parses the `.lyst` config file used by logging and build defaults
- `src/wizard.c` creates or edits `.lyst`
- `src/tests.c` provides the smoke test flow

### 1.2 Public Headers And Shared Types
The `include/` directory is where the shared contracts live. The most important headers are:

- `include/type.h` for tokens and token streams
- `include/parser.h` for bytecode results, constant values, globals, and parser state
- `include/bytecode.h` for opcode values
- `include/headerer.h` for the file header layout
- `include/errors.h` for logging and fatal error helpers
- `include/args.h` for CLI parsing
- `include/lyst.h` for configuration loading
- `include/vm.h` for the VM entry point

The code is easiest to understand when you treat these headers as the real contract and read the `.c` files as the implementation.

### 1.3 The Current Repository Shape
The top-level tree currently contains:

- source in `src/`
- headers in `include/`
- design notes in `docs/`
- older experimental code in `archive/`
- sample source in `test.leyo`
- sample compiled output in `test.lybc`
- logs in `logs/`
- the config example in `.lyst`
- the project roadmap in `TODO.md`
- contributor notes in `CONTRIBUTING.md`

The older `archive/` code is useful as history, but the live behavior is the code in `src/` and the contract in `include/`.

### 1.4 Reading Order For A New Employee Or Intern
If someone is trying to get oriented quickly, the best path is:

1. `README.md`
2. `docs/cli.md`
3. `src/main.c`
4. `src/helper.c`
5. `include/args.h` and `src/args.c`
6. `src/lexer.c`
7. `src/parser.c`
8. `include/bytecode.h`
9. `src/headerer.c`
10. `src/runner.c`
11. `src/vm.c`
12. `src/errors.c`
13. `src/lyst.c`
14. `src/wizard.c`
15. `src/tests.c`

That order mirrors the actual data flow: CLI -> config -> compile or run -> bytecode -> VM.

### 1.5 The Current State Of The Docs
The docs are useful, but they are not perfectly synchronized with the live code. A few examples:

- `src/helper.c` advertises commands that are more current than the top-level README text
- `main.c` handles `update`, but the help table does not list it
- `include/bytecode.h` defines `OP_JUMP`, but the VM and disassembler do not fully support it yet
- `src/repl.c` currently returns an error before the interactive loop can run
- `src/parser.c` has function-scaffolding code, but function support is still incomplete

For day-to-day work, trust the source over legacy wording.

### 1.8 What A New Contributor Should Watch Closely
There are a few spots where the code is especially important to read before making changes:

- `src/helper.c` because it defines the actual CLI help surface
- `src/main.c` because it shows the real command dispatch order
- `src/parser.c` and `src/vm.c` because bytecode shape lives between them
- `src/errors.c` because every failure path eventually passes through it
- `src/wizard.c` and `src/lyst.c` because config, logging, and build defaults all depend on them

If these files stay aligned, the rest of the project is much easier to reason about.

## 2. The Command-Line Interface

### 2.0 CLI Entry Point
`src/main.c` is the top of the runtime. It creates an `ArgParser`, decides which command the user asked for, loads config when needed, initializes logging, and dispatches into the compiler, runner, disassembler, or support utilities.

### 2.1 `argParseSetup` And The CLI Shape
`src/args.c` implements a deliberately simple parser:

- the first non-flag argument becomes the command
- arguments starting with `-` become either flags or options
- if a `-something` argument is followed by a non-flag token, it is stored as an option key/value pair
- otherwise it is stored as a plain flag
- any other tokens are positional arguments

The parser does not try to be clever. It is a small hand-rolled splitter that preserves the raw strings and makes the rest of the program easy to inspect.

### 2.1.1 Fields Stored In `ArgParser`
`include/args.h` defines the parser state:

- `command` - the command name such as `build` or `run`
- `noCommand` - true when the user passed only flags or nothing
- `flags` and `flagAmount`
- `positionals` and `positionalAmount`
- `optionKeys`, `optionValues`, and `optionAmount`
- `bin` - the executable path from `argv[0]`

That layout explains why `main.c` can ask for either a positional source file or a `-s` script option when building.

### 2.2 The Help Surface In `src/helper.c`
`src/helper.c` owns the text printed by `leyo help`. This file is more important than it looks because it defines what the CLI claims to support.

### 2.2.1 Help Table Entries
The help table currently contains these commands:

- `help [query]`
- `init [--defaults]`
- `build <source> [output]`
- `run <file.lybc>`
- `repl`
- `test`
- `disassemble <file.lybc> [--hex] [--head]`
- `dis <file.lybc> [--hex] [--head]`
- `do`
- `github`

### 2.2.2 How The Search Works
`printHelp(const char *query)` does three things:

1. prints a header and the generic `leyo <command> [args]` usage line
2. loops through the static command table
3. prints only entries whose name, usage, or description contains the query string, case-insensitively

The matcher is a substring search, not a shell-style glob and not a regex. An empty query shows everything.

### 2.2.3 What The Helper Tells New Users
This file is also a clean description of the intended CLI:

- `help` is searchable
- `init --defaults` writes a default config without prompting
- `dis` is just a shorter alias for `disassemble`
- `do` uses paths from `.lyst`
- `github` opens the project page

That is the user-facing shape of the tool as the project sees it today.

### 2.3 Command Dispatch In `main.c`
The command flow in `main.c` is the central connection point between modules.

### 2.3.1 Startup Sequence
When the program starts, it:

1. initializes `ArgParser`
2. checks for `help`
3. checks for `init`
4. loads `.lyst`
5. reads logging settings from `.lyst`
6. initializes logging
7. handles global flags if no command was provided
8. dispatches into the selected command

That order matters. `help` and `init` intentionally bypass the config load so the user can still get help or generate config when no `.lyst` exists yet.

### 2.3.2 Global Flags
When no command is provided, the CLI recognizes:

- `--version` or `-v`
- `--diagnostics` or `-D`

If no flags are provided at all, the program prints a short version banner. The version string comes from `include/version.h`, which is generated from `VERSION.mk` by the Makefile.

### 2.3.3 Command Dispatch Table
The active command paths are:

- `build`
- `run`
- `repl`
- `test`
- `disassemble`
- `dis`
- `do`
- `github`
- `update`

`update` is present in the dispatcher but only logs that it is checking for updates. It is a stub right now.

### 2.4 Exact Behavior Of Each CLI Command

### 2.4.1 `help`
`help` calls `printHelp(getPositional(&parser, 0))`. The first positional argument becomes the search filter.

### 2.4.2 `init`
`init` either:

- writes the default `.lyst` when `--defaults` or `-y` is present
- or launches the interactive wizard

### 2.4.3 `build`
`build` accepts:

- a positional source file
- or `-s <script>` for direct source text
- optional `-o <output>` for the destination file

If no output path is provided, it defaults to `a.lybc`.

### 2.4.4 `run`
`run` expects a compiled `.lybc` file and hands it to the runtime loader in `src/runner.c`.

### 2.4.5 `repl`
`repl` is currently stubbed. It logs a request and aborts with `Repl Not Built` before reaching the unfinished interactive loop below it.

### 2.4.6 `test`
`test` runs the smoke-test flow in `src/tests.c`, using the current executable path from the argument parser.

### 2.4.7 `disassemble` And `dis`
These commands open a `.lybc` file and print it either as readable opcodes or as hex bytes. `--hex` switches to hex mode, and `--head` skips the header before disassembly.

### 2.4.8 `do`
`do` pulls `build/in` and `build/out` from `.lyst` and feeds them into the build pipeline.

### 2.4.9 `github`
`github` opens the project repository in the system browser using platform-specific shell commands.

### 2.4.10 `update`
`update` is intentionally minimal for now. It logs the intention but does not perform a real update flow yet.

### 2.5 Function Connection Map For The CLI
The main call chains look like this:

- `main` -> `argParseSetup`
- `main` -> `printHelp`
- `main` -> `writeDefaultLyst` or `runInitWizard`
- `main` -> `lystLoad` -> `readLogConfig` -> `setLogConfig` -> `initLog`
- `main` -> `build` -> `tokenise` -> `parse` -> `headThis`
- `main` -> `run` -> `runVM`
- `main` -> `dis` -> `disassemble` or `disassembleHex`
- `main` -> `testLeyo`
- `main` -> `openGithub`

That is the real skeleton of the project.

## 3. Configuration And Logging

### 3.0 Configuration And Logging
The configuration path is a substantial part of the project. It is not just a side feature: the CLI, the logger, and the `do` command all depend on it.

### 3.1 `.lyst` File Parsing In `src/lyst.c`
`src/lyst.c` implements a small section/key/value parser.

### 3.1.1 Supported File Shape
The parser expects a file that looks like:

- `[section]`
- `key = value`

It ignores blank lines, `#` comments, and `;` comments.

### 3.1.2 What Gets Stored
Each entry becomes a `LystEntry` with:

- `section`
- `key`
- `value`

All entries are stored in memory inside `LystConfig`.

### 3.1.3 Access Helpers
The config layer exposes:

- `lystGet("section/key")`
- `lystGetBool("section/key", fallback)`
- `lystGetInt("section/key", fallback)`

The lookup is simple string matching against the parsed entries.

### 3.2 Logging Configuration In `src/wizard.c`
`src/wizard.c` turns user input into a `.lyst` file and also reads logging defaults back into a `LogConfig`.

### 3.2.1 `runInitWizard`
The wizard:

1. explains that Enter accepts defaults
2. offers to overwrite an existing `.lyst`
3. prompts for logging switches
4. asks for log retention behavior
5. asks for the active log path
6. asks for the archive directory
7. asks for the default build input and output files
8. writes `.lyst`

### 3.2.2 `writeDefaultLyst`
This writes the same default structure without prompting:

- logging enabled
- rotation enabled
- build/runtime/controller/error logs enabled
- 30-day retention
- archive old logs
- log path `logs/latest.lylog`
- archive path `logs/archive/`
- default build input `test.leyo`
- default build output `test.lybc`

### 3.2.3 `readLogConfig`
This reads the `.lyst` values and produces a `LogConfig` for the logging subsystem. If values are missing, it falls back to safe defaults.

### 3.3 The Logger And Error System In `src/errors.c`
`src/errors.c` owns both logging and fatal error reporting.

### 3.3.1 Log Categories
The logger supports these categories:

- build logs
- runtime logs
- controller logs
- error logs

Each category is controlled by the config.

### 3.3.2 Rotation And Retention
The logger can:

- rotate the active log on startup
- sweep older `.lylog` files
- archive old logs into an archive directory
- or delete them, depending on config

This is why the repo keeps logs under versioned and timestamped filenames instead of overwriting them blindly.

### 3.3.3 `raise` And `callAllErr`
The error path is straightforward:

- `raise` records the error and marks the global `isErr` flag
- if strict mode is enabled, it immediately calls `callAllErr`
- `callAllErr` prints every queued error, closes the log, and exits

That means many modules can raise a failure without needing to know how to print it.

### 3.3.4 Log Initialization
`initLog` opens the configured log file, rotates the old one if requested, creates missing parent directories, and runs log retention maintenance.

### 3.4 Diagnostics
`src/diagnostics.c` prints build metadata:

- project version
- build date and time
- compiler version
- platform
- architecture
- dirty/clean git state
- git commit
- `sizeof(Value)`

This is what `--diagnostics` uses when the CLI has no command.

## 4. The Build Pipeline

### 4.0 The Build Pipeline
The build flow is the path most contributors will touch first.

### 4.1 `src/build.c`
`build(char *filename, char *bcrfilename, bool isFlnameScript)` is the front-end compiler entry point.

### 4.1.1 Input Sources
It can build from either:

- a source file on disk
- or an in-memory script string when `isFlnameScript` is true

### 4.1.2 Build Sequence
The function:

1. loads source text into memory when needed
2. calls `tokenise`
3. prints the token stream for visibility
4. checks for lexer errors
5. calls `parse`
6. wraps the returned bytecode with `headThis`
7. prints the resulting raw bytecode bytes
8. writes the `.lybc` file to disk

### 4.1.3 Why It Is Useful For New Developers
`build.c` is the cleanest place to see the compiler boundary. It connects the lexer, parser, header wrapper, and file output in one place.

### 4.2 The Lexer In `src/lexer.c`
The lexer is a small state machine with these modes:

- normal
- string
- number
- identifier
- comment

### 4.2.1 Tokens It Produces
`include/type.h` defines the token kinds. The lexer currently recognizes:

- identifiers
- numbers
- floats
- strings
- chars
- operators
- conditions
- equals
- commas
- semicolons
- braces
- brackets
- native-call markers
- end-of-stream

### 4.2.2 Source Rules
The lexer currently behaves like this:

- whitespace is skipped
- `~` opens and closes comments
- double quotes start strings
- single quotes produce one-character `CHR` tokens
- letters start identifiers
- digits start numbers
- `@` becomes a native token
- `+ - * / ^` become operation tokens
- `> < !` become condition tokens
- `=` can become `=` or `==` or `===` depending on following characters

### 4.2.3 Helper Functions
The lexer is built from small helpers:

- `charIn`
- `isAlpha`
- `isInt`
- `_token`
- `token`
- `handleNormal`
- `handleString`
- `handleIdentifier`
- `handleNumber`
- `handleComment`
- `tokenise`

The header also declares `strIn`, but the current source does not implement it.

### 4.2.4 The End-Of-Stream Marker
Every token stream ends with a synthetic `EndOfStream` token. The parser relies on that sentinel.

### 4.3 Parser Data Structures
`include/parser.h` defines the parser-facing runtime structures.

### 4.3.1 `Value`
`Value` is the runtime representation for literals and constant-pool entries. It can hold:

- int
- float
- char
- string

### 4.3.2 `Global`
`Global` tracks named variables with:

- `name`
- `slot`
- `type`

### 4.3.3 `Func`
`Func` is the scaffold for function support. It stores:

- `name`
- `address`
- `retType`

Function arguments are marked as a future addition.

### 4.3.4 `ByteCoder`
This is the parser's working state:

- token stream pointer
- current token index
- token count
- byte buffer
- byte index
- global table
- constant table
- function table

`ByteCoder` is where the parser keeps track of all compile-time state.

### 4.4 Parsing Flow In `src/parser.c`
The parser has a classic structure:

- read the current token
- inspect the next token
- emit bytes into the instruction buffer
- resolve names against tables
- serialize constants into a separate buffer

### 4.4.1 Core Helpers
The parser uses:

- `current`
- `peek`
- `advance`
- `expectCurrent`
- `expect`
- `expectAndPass`
- `emit`
- `emit16`
- `emit32`
- `constEmit`

These helpers are the backbone of the whole compiler side.

### 4.4.2 Type Mapping
`getTypeVar` maps type keywords to token categories:

- `int` -> `NUMBER`
- `str` -> `STRING`
- `flt` -> `FLT`
- `chr` -> `CHR`

That is how the parser knows what kind of value a declaration is supposed to hold.

### 4.4.3 Constant Serialization
The parser serializes constants separately from instructions:

- `0x01` for int
- `0x02` for float
- `0x03` for char
- `0x04` for string

The VM decodes that exact format later.

### 4.4.4 Variable And Function Tables
The parser has helpers for:

- `define`
- `definef`
- `resolve`
- `resolvef`
- `resolveType`
- `resolveTypef`

These functions connect source names to bytecode slots or function addresses.

### 4.4.5 Expression Parsing
`parseExpression` handles left-to-right binary expressions and currently supports:

- `+`
- `-`
- `*`
- `/`
- `^`

Atoms can be:

- numbers
- floats
- chars
- strings
- identifiers
- function calls

The parser emits instructions that use the A/B/R register convention in the VM.

### 4.4.6 Statements
`parseStatement` dispatches by token class and recognizes:

- native calls
- variable declarations
- assignments
- function declarations
- function calls

### 4.4.7 Native Calls
`parseNative` currently recognizes:

- `@log`
- `@dump`
- `@trace`

It emits `OP_CALL_NATIVE` with the selected native command.

### 4.4.8 Functions Are Scaffolded, Not Finished
`parseFunction` records the function name and return type, but parameter handling is still marked TODO. Function calls are also incomplete. The parser emits `OP_JUMP` for calls, but the VM does not implement that opcode yet.

### 4.5 The Build Output Structure
`parse` returns a `ByteCodeResult` containing:

- `data` for the instruction stream
- `cb` for the serialized constant pool

`headThis` then prefixes the Leyo file header and combines the streams into the final `.lybc` payload.

## 5. Bytecode And File Format

### 5.0 Bytecode And File Format
The file format is small enough to understand in one sitting, which is one of the nicest things about the project.

### 5.1 Opcode Definitions In `include/bytecode.h`
The current opcode table contains:

- register setup: `OP_PUT_A`, `OP_PUT_B`, `OP_PUT_S`, `OP_PUT_A_R`, `OP_PUT_B_R`
- arithmetic: `OP_OPERATE_MUL`, `OP_OPERATE_DIV`, `OP_OPERATE_ADD`, `OP_OPERATE_SUB`, `OP_OPERATE_EXP`
- globals: `OP_STORE`, `OP_LOAD`
- speed stack helpers: `OP_SS_PUSH_A`, `OP_SS_PUSH_B`, `OP_SS_POP`
- constants: `OP_CONST_LOAD`
- native calls: `OP_CALL_NATIVE`
- jumps: `OP_JUMP`
- program terminator: `OP_FINISH`

Some of these are defined ahead of full implementation. In particular, the speed stack opcodes and `OP_JUMP` are part of the bytecode vocabulary even when not all consumers support them yet.

### 5.2 Header Layout In `include/headerer.h`
The Leyo file header is:

- `magic[4]`
- `version[64]`
- `flags`
- `code_size`

The magic is `LYBC`. The version string comes from `LEYO_VERSION`.

### 5.3 How `headThis` Builds The Final File
`src/headerer.c` constructs the header, allocates a new buffer, and copies data in this order:

1. header
2. instruction bytes
3. constant-pool bytes

That layout is what the runtime loader expects.

### 5.4 What `run` Expects To Read
`src/runner.c` reverses the process:

1. open the file
2. read the header
3. verify `LYBC`
4. use `code_size` to split code from constants
5. hand the buffers to `runVM`

This means the compiler and runtime must stay in lockstep whenever the format changes.

### 5.5 A Practical Detail About The Disassembler
`src/disassembler.c` uses a fixed entry point of `0x4C` when `--head` is set. That is the current header-size assumption baked into the tool.

## 6. The Virtual Machine

### 6.0 The Virtual Machine
The VM is small but important. It is the point where the abstract compiler output becomes behavior.

### 6.1 Runtime State In `src/vm.c`
The VM tracks:

- instruction bytes
- instruction pointer
- registers `A`, `B`, and `R`
- a speed stack
- a global variable array
- a decoded constant pool

The speed stack has a fixed size of 256. The globals array is very large, sized for 65,536 slots.

### 6.2 Constant Pool Decoding
`runVM` decodes the constant pool before execution when one is present.

The encoded forms are:

- `0x01` -> int
- `0x02` -> float
- `0x03` -> char
- `0x04` -> string

This decoder must match the parser's serializer exactly.

### 6.3 Opcode Execution Model
The VM reads one opcode at a time and dispatches through a `switch`.

### 6.3.1 Register Setup
- `OP_PUT_A` reads a 16-bit immediate into `A`
- `OP_PUT_B` reads a 16-bit immediate into `B`
- `OP_PUT_S` swaps `A` and `B`
- `OP_PUT_A_R` copies `R` into `A`
- `OP_PUT_B_R` copies `R` into `B`

### 6.3.2 Arithmetic
The math opcodes route through dedicated helper functions:

- `addition`
- `subtraction`
- `multiplication`
- `division`
- `power`

These helpers handle int/float combinations and reject string/char arithmetic with runtime errors.

### 6.3.3 Globals
- `OP_STORE` writes `A` into the global slot stored in `B`
- `OP_LOAD` reads a global slot from `A` into `R`

### 6.3.4 Constants
- `OP_CONST_LOAD` reads a constant-pool index and copies the value into `R`

### 6.3.5 Native Calls
`OP_CALL_NATIVE` reads a `NativeCommand`. Right now the VM only handles `NAT_DUMP`, which prints the current register and stack state.

### 6.3.6 Program End
`OP_FINISH` frees the constant pool and exits successfully.

### 6.4 What The VM Does Not Yet Do
The VM does not currently implement every declared bytecode feature. The obvious examples are:

- `OP_JUMP`
- the speed stack opcodes
- richer native command handling beyond `NAT_DUMP`

That is normal for the current project stage, but it matters when reading the parser output.

## 7. Disassembly And Debugging

### 7.0 Disassembly And Debugging
These tools exist so contributors can inspect behavior without guessing.

### 7.1 `src/disassembler.c`
The disassembler has two output modes:

- readable opcode names
- raw hex bytes

`disassemble` prints opcodes by name, and `disassembleHex` prints bytes directly.

### 7.2 `dis`
`dis(char *filename, bool flag_justHex, bool flag_head)`:

1. opens the target file
2. reads all bytes
3. optionally skips the header
4. prints either opcode names or hex bytes

It is a quick way to verify what the compiler emitted.

### 7.3 `src/tests.c`
The smoke test is a very practical integration check:

1. create a temporary folder
2. write a tiny `test.leyo`
3. build it using the current executable
4. confirm the `.lybc` file exists
5. run the bytecode
6. disassemble it in hex mode
7. clean up the temporary files

It uses the executable path from the CLI parser, so the test exercises the real built binary, not an imagined one.

### 7.4 `src/diagnostics.c`
Diagnostics print the environment details a maintainer usually needs when reporting or reproducing issues:

- version
- build date
- compiler
- platform
- architecture
- git state
- `Value` size

## 8. Build System

### 8.0 Build System And Generated Files
The Makefile is part of the project story because it generates some of the headers the code depends on.

### 8.1 `Makefile`
The Makefile:

- compiles every `src/*.c`
- writes objects into `build/`
- links the executable into `bin/`
- regenerates `include/version.h` from `VERSION.mk`
- injects `GIT_COMMIT` and `GIT_DIRTY` into the build

That is why `main.c` and `diagnostics.c` can print version and git state without reading git at runtime.

### 8.2 `VERSION.mk` And `include/version.h`
`VERSION.mk` is the source of truth for the project version. The generated `include/version.h` defines `LEYO_VERSION`, which is used in:

- the CLI banner
- diagnostics
- the file header version string

### 8.3 Log And Config Files
The live project also uses:

- `.lyst` for config
- `logs/latest.lylog` for active logs
- `logs/archive/` for archived logs

The sample `.lyst` in the repo shows the current default keys and values.

## 9. Development Notes

### 9.0 Current Gaps And Practical Caveats
This section is the part a newcomer usually wishes someone had said out loud.

### 9.1 Things That Are Solid
These paths are currently the most coherent:

- lexer
- basic variable declarations
- assignments
- numeric and string/char literal capture
- bytecode wrapping and loading
- VM execution of the current arithmetic/global/constant opcodes
- logging and config loading
- the smoke test flow

### 9.2 Things That Are In Progress
These areas are scaffolded or only partially wired:

- function declarations and calls
- `OP_JUMP` handling
- REPL
- richer native commands
- a more complete test suite
- some CLI commands mentioned in older docs

### 9.3 Important Source-Of-Truth Rules
When changing this project, these relationships must stay aligned:

- lexer tokens and parser expectations
- parser-emitted opcodes and VM opcode handling
- constant serialization and VM decoding
- header layout and file loading
- `.lyst` keys and logging/config code
- CLI help text and actual command dispatch

If one of those pairs drifts, the project becomes harder to understand and debug very quickly.

## 10. Summary

### 10.0 The Short Version
Leyo is a compact compiler/runtime stack written in C. `main.c` dispatches commands, `helper.c` defines the help text, `args.c` shapes the CLI, `lexer.c` tokenizes source, `parser.c` emits bytecode and constants, `headerer.c` packages the file, `runner.c` loads it back, `vm.c` executes it, and `errors.c` keeps logging and fatal reporting centralized. The codebase is small enough that every part is visible, and the right way to work on it is to keep those parts synchronized rather than treating them as separate subsystems.
