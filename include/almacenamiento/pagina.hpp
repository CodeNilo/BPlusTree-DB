#include <cstdint>
#include <cstddef>
#pragma once

constexpr size_t PAGINA_SIZE = 4096;

using PaginaID = uint32_t;
constexpr PaginaID INVALID_PAGE_ID = 0xFFFFFFFF;

// Leemos el primer byte para saber el tipo de Nodo
enum class TipoPagina : uint8_t { 
    NO_DEFINIDA = 0,
    INTERNA = 1,
    HOJA = 2 
};

// En c++ si importa el orden de declaracion
struct HeaderPagina {
    TipoPagina tipo; // Byte 0
    uint8_t nivel;  // Byte 1
    uint16_t num_celdas; // Byte 2 y 3
};