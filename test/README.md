# Test - Bulk Insert

Este directorio contiene herramientas de prueba para la base de datos PeruDB.

## bulk_insert.cpp

Programa para insertar grandes cantidades de datos aleatorios en la base de datos.

### Compilacion

```bash
g++ -g -Wall -Wextra -std=c++17 -I../include test/bulk_insert.cpp src/database.cpp src/index/bplustree.cpp src/almacenamiento/archivo_mapeado_memoria.cpp src/almacenamiento/paginador.cpp src/almacenamiento/pagina_ranurada.cpp -o test/bulk_insert.exe
```

### Uso

```bash
./test/bulk_insert.exe <archivo.db> <cantidad_registros>
```

### Ejemplos

Insertar 1000 registros:
```bash
./test/bulk_insert.exe test_data.db 1000
```

Insertar 100,000 registros:
```bash
./test/bulk_insert.exe test_data.db 100000
```

Insertar 1,000,000 registros:
```bash
./test/bulk_insert.exe test_data.db 1000000
```

### Notas

- El programa genera DNIs aleatorios entre 10,000,000 y 99,999,999
- Los nombres, apellidos y direcciones se generan aleatoriamente de listas predefinidas
- Si un DNI ya existe, intenta hasta 5 veces con DNIs diferentes
- Muestra progreso cada 10,000 registros insertados
- Al final muestra estadisticas de tiempo y velocidad de insercion
