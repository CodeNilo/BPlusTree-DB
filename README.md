# B-Tree-DB

A simple B+ tree database implementation in C++ with memory-mapped storage.

## Features

- B+ tree indexing for efficient lookups and range queries
- Memory-mapped file I/O for fast data access
- Page-based storage with slotted page layout
- Support for variable-length records

## Building

```bash
g++ -g -Wall -Wextra -std=c++17 -Iinclude \
    src/main.cpp \
    src/database.cpp \
    src/index/bplustree.cpp \
    src/almacenamiento/archivo_mapeado_memoria.cpp \
    src/almacenamiento/paginador.cpp \
    src/almacenamiento/pagina_ranurada.cpp \
    test/generador_datos.cpp \
    -o build/db.exe
```

## Usage

```bash
./build/db.exe db.bplustree
```

## Structure

- `include/` - Header files
- `src/` - Implementation files
  - `almacenamiento/` - Storage layer (pages, pager, memory-mapped files)
  - `index/` - B+ tree implementation
- `test/` - Test utilities and data generators
