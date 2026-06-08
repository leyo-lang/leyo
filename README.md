# 📘 Leyo Language

## Introduction
**Leyo** is a fully observant, permissive programming language designed with security, traceability, and controlled execution in mind. It logs all actions performed within the system and enforces strict type safety to ensure only explicitly allowed code can run.

Its syntax is inspired by C-style languages, while introducing simplified constructs—such as array handling—and deliberately removing unsafe features like pointers.

---

## Table of Contents
- Introduction  
- Features  
- Syntax Overview  
- Installation  
- Usage  
- Logger Configuration  
- Security Model  
- Troubleshooting  
- Contributors  
- License  

---

## Features
- 🔍 **Full Observability** – Every action is logged automatically  
- 🔐 **Permission-Based Execution** – Code must explicitly declare allowed actions  
- ✅ **Type Safety** – Strong typing prevents unsafe operations  
- 🚫 **No Pointers** – Eliminates a major class of memory errors  
- 🧩 **C-like Syntax** – Familiar structure for developers  
- 📦 **Simplified Arrays** – Easier data handling without complex memory management  

---

## Syntax Overview
Leyo uses a structured, declarative style where permissions and behaviors are explicitly defined.

### Key Concepts
- `allow` — Grants permission for specific operations  
- `defined` — Declares a function or behavior  
- `log` — Outputs observable logs  
- `pass` — Allows execution to proceed  
- `***` — Comment syntax  

---

## Installation
Leyo can be installed at <https://github.com/JoshRuds/leyo/releases> for free.
It is currently still WIP so any errors can be added as an issue on the [Github Page](https://github.com/JoshRuds/leyo)

---

## Usage  
`leyo (command) [additional information] [flags]`

- No Commands — displays version number and build number
- build {filename} {optional: destination} [flags: none implemented yet] — builds file `.leyo` into destination `.lybc`
- disassemble {filename} [flags: none implemented yet] — outputs the `.lybc` file from binary into human readable commands
- run {filename} [flags: none implemented yet] — runs `.lybc` file 
- hash — checks own hash againts publicly available hashes to ensure untampered executable
- help — outputs helpfile for commands
- log {filename} — sets up `.lylog` file for later logging of actions

---

## Logger Configuration
Configuration is available via `leyo log` command

---

## Security Model
Leyo enforces a **permission-first execution model**:
- No operation runs unless explicitly allowed
- All actions are logged for auditing
- Type safety prevents undefined behavior

---

## Troubleshooting
Please drop an issue on the [Github Page](https://github.com/JoshRuds/leyo)

---

## Contributors
Josh Ruddick — Founder

---

## License
[Apache-2.0](https://github.com/JoshRuds/leyo/blob/main/LICENSE)
