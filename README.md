# Peril
A C++ library built around the Java Virtual Machine 17 Specification

## Goals
- A Virtual Machine that can execute Java 17 bytecode
- Tools and visualization aids (disassembler, decompiler, etc)

## Non-Goals
- Compiler

## Build
Use `cmake` (and preferably `ninja`).
```bash
mkdir Build
cd Build
cmake -G Ninja ..
ninja
```
