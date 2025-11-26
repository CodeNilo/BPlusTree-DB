#include "almacenamiento/paginador.hpp"
#include "core/ciudadano.hpp"
#include "core/types.hpp"
#include <vector>
#include <optional>

#pragma once

enum class TipoNodo : uint8_t {
    Interno = 0,
    Hoja = 1,
};

struct BPlusTreeHeader {
    TipoNodo tipo;
    uint16_t num_claves;
};

namespace Hoja {
    struct Entrada {
        DNI_t clave;
        RegistroID valor;
    };
    // Cálculo correcto: Página - Header - PunteroSiguiente / TamañoEntrada
    constexpr int MAX_CLAVES = (PAGINA_SIZE - sizeof(BPlusTreeHeader) - sizeof(PaginaID)) / sizeof(Entrada);
    // Con 4KB páginas: (4096 - 3 - 4) / (4 + 8) = 340 entradas por hoja
}

namespace Interno {
    // Orden optimizado para 30M registros
    // Fórmula: (PAGINA_SIZE - Header) / (sizeof(PaginaID) + sizeof(DNI_t))
    constexpr int ORDEN = (PAGINA_SIZE - sizeof(BPlusTreeHeader)) / (sizeof(PaginaID) + sizeof(DNI_t));
    constexpr int MAX_CLAVES = ORDEN - 1;
    // Con 4KB: (4096 - 3) / (4 + 4) = 511 orden, 510 claves por nodo interno
    // Altura del árbol con 30M registros: log_511(30M/340) ≈ 3 niveles
}

class BPlusTree {
public:
    BPlusTree(Paginador& paginador);

    PaginaID inicializar(PaginaID id_raiz);

    std::optional<RegistroID> buscar(DNI_t clave);
    
    bool insertar(DNI_t clave, RegistroID valor);

    bool eliminar(DNI_t clave);

    PaginaID get_id_raiz() const;

private:
    Paginador& paginador;
    PaginaID id_raiz;

    std::optional<RegistroID> buscar_en_nodo(PaginaID id_pagina, DNI_t clave);

    struct ResultadoDivision {
        DNI_t clave_promocionada;
        PaginaID id_nueva_pagina;
    };

    std::optional<ResultadoDivision> insertar_en_nodo(PaginaID id_pagina, DNI_t clave, RegistroID valor);
    
    void insertar_en_hoja(char* pagina_ptr, DNI_t clave, RegistroID valor);
    
    void insertar_en_interno(char* pagina_ptr, DNI_t clave, PaginaID id_hijo_derecho);
    
    PaginaID buscar_hoja(DNI_t clave);

    // === Helpers para Eliminación ===
    void eliminar_interno(PaginaID id_pagina, DNI_t clave, PaginaID id_padre, int indice_en_padre);

    // Estructura para encontrar hermanos
    enum class DireccionHermano { Izquierdo, Derecho };
    struct InfoHermano {
        PaginaID id;
        int indice_en_padre;
        DireccionHermano direccion;
    };
    std::optional<InfoHermano> buscar_hermano(PaginaID id_padre, int indice_en_padre);

    // Operaciones en Nodos Hoja
    void redistribuir_hojas(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre);
    void fusionar_hojas(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre);

    // Operaciones en Nodos Internos
    void redistribuir_internos(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre);
    void fusionar_internos(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre);
};
