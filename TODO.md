# TODO

This is the current working list for Leyo. I’m keeping it short, concrete, and ordered by what will most improve the project for contributors.

## High Priority

- [ ] Add a real `help` command to the CLI
- [ ] Add a `version` command with build metadata
- [ ] Add automated smoke tests for build/run/disassemble
- [ ] Expand parser coverage beyond simple literals and identifiers
- [ ] Add better runtime error messages for VM opcode failures
- [ ] Document the bytecode header layout in a machine-readable way

## Medium Priority

- [ ] Add `build --debug` or `build --trace` for compiler diagnostics
- [ ] Add `run --trace` for opcode-level VM stepping
- [ ] Add source formatting support once syntax is stable
- [ ] Add a minimal standard library for common language features
- [ ] Add a proper sample program suite under `examples/`
- [ ] Add regression tests for lexer, parser, and bytecode generation

## Lower Priority

- [ ] Add JSON disassembly output
- [ ] Add a package or project manifest format
- [ ] Add REPL improvements
- [ ] Add command aliases for frequently used flows
- [ ] Clean up older archive code once the new paths are fully stable

## Done Or In Progress

- [ ] Bytecode build/run alignment
- [ ] Constant pool serialization and VM loading
- [ ] Contributor docs refresh
- [ ] CLI reference docs
