# Leyo Roadmap

MAINTAINERS: UPDATE AFTER COMPLETION

## Core Language

- [x] Variables
- [x] Expressions
- [x] Type system
- [x] Functions
- [x] Strings

### Execution
- [ ] Stack VM
- [ ] Call frames
- [ ] Native functions
- [ ] Exit codes

### Scopes & Lifetime
- [ ] Scope handling
- [ ] Local variables
- [ ] Global variables
- [ ] Constant variables
- [ ] Shadowing rules

### Control Flow
- [ ] `if`
- [ ] `else`
- [ ] `else if`
- [ ] `while`
- [ ] `repeat`
- [ ] `for`
- [ ] `switch`
- [ ] `match` (optional)
- [ ] `break`
- [ ] `continue`
- [ ] `return`

### Types
- [ ] Arrays
- [ ] Tuples
- [ ] Structs
- [ ] Enums
- [ ] Unions (optional)
- [ ] Type aliases
- [ ] Generics (optional)
- [ ] Nullable types
- [ ] Type inference
- [ ] Casting

### Functions
- [ ] Default parameters
- [ ] Variadic parameters
- [ ] Function overloading (optional)
- [ ] Anonymous functions
- [ ] Closures

---

## Compiler

### Lexer
- [ ] Unicode identifiers
- [ ] Escape sequences
- [ ] Numeric literals
- [x] Comments

### Parser
- [ ] Error recovery
- [ ] Better diagnostics
- [ ] AST validation

### Semantic Analysis
- [x] Undefined variable detection
- [ ] Unused variable warnings
- [ ] Type checking
- [ ] Constant folding
- [ ] Dead code detection

### Bytecode Compiler
- [ ] Function compilation
- [ ] Jump patching
- [ ] Constant pool improvements
- [ ] Debug information

---

## Virtual Machine

- [ ] Memory management
- [ ] Heap allocator
- [ ] Garbage collector
- [ ] String interning
- [ ] Stack overflow protection
- [ ] Runtime type checking
- [ ] Exception handling
- [ ] Native API

### Bytecode Optimizations
- [ ] Peephole optimizer
- [ ] Constant propagation
- [ ] Dead code elimination
- [ ] Instruction combining
- [ ] Inline caching (future)

---

## Modules

- [x] Modules
- [x] Imports
- [ ] Public/private visibility
- [ ] Circular import detection

---

## Standard Library

- [ ] `std.io`
- [ ] `std.math`
- [ ] `std.string`
- [ ] `std.array`
- [ ] `std.time`
- [ ] `std.random`
- [ ] `std.fs`
- [ ] `std.path`
- [ ] `std.process`
- [ ] `std.net`
- [ ] `std.json`
- [ ] `std.collections`

---

## Tooling

- [ ] Formatter
- [ ] Linter
- [ ] REPL
- [ ] Documentation generator
- [ ] Language Server (LSP)
- [ ] Syntax highlighting
- [ ] Debugger
- [x] Bytecode disassembler

---

## CLI

- [ ] `leyo run`
- [ ] `leyo build`
- [ ] `leyo compile`
- [ ] `leyo format`
- [ ] `leyo lint`
- [ ] `leyo repl`
- [ ] `leyo test`
- [ ] `leyo doc`
- [ ] `leyo version`

---

## Testing

- [ ] Unit tests
- [ ] VM tests
- [ ] Parser tests
- [ ] Compiler tests
- [ ] Standard library tests
- [ ] Regression tests
- [ ] Fuzz testing

---

## Performance

- [ ] Faster lexer
- [ ] Faster parser
- [ ] Faster VM dispatch
- [ ] Memory optimizations
- [ ] Reduced allocations
- [ ] Benchmark suite

---

## Ecosystem

- [ ] Package manager
- [ ] Package registry
- [ ] Build system
- [ ] Dependency resolution
- [ ] Version locking

---

## Release

- [ ] Windows support
- [ ] Linux support
- [ ] macOS support
- [ ] CI/CD
- [ ] Documentation website
- [ ] Examples
- [ ] Tutorial
- [ ] v1.0 Release