#include "almacenamiento/archivo_mapeado_memoria.hpp"
#include "almacenamiento/pagina.hpp"
#include <set>
#include <cstddef>
#include <unordered_map>
#include <list>

#pragma once

// Caché LRU simple para páginas frecuentemente accedidas
class CacheLRU {
private:
    struct NodoCache {
        PaginaID pagina_id;
        char* datos;
    };

    size_t capacidad;
    std::list<NodoCache> lista_lru;
    std::unordered_map<PaginaID, typename std::list<NodoCache>::iterator> mapa;

public:
    CacheLRU(size_t capacidad) : capacidad(capacidad) {}

    char* get(PaginaID page_id) {
        auto it = mapa.find(page_id);
        if (it == mapa.end()) {
            return nullptr; // No está en caché
        }

        // Mover al frente (más recientemente usado)
        lista_lru.splice(lista_lru.begin(), lista_lru, it->second);
        return it->second->datos;
    }

    void put(PaginaID page_id, char* datos) {
        auto it = mapa.find(page_id);

        if (it != mapa.end()) {
            // Ya existe, actualizar y mover al frente
            it->second->datos = datos;
            lista_lru.splice(lista_lru.begin(), lista_lru, it->second);
            return;
        }

        // Verificar capacidad
        if (lista_lru.size() >= capacidad) {
            // Eliminar el menos recientemente usado
            auto ultimo = lista_lru.back();
            mapa.erase(ultimo.pagina_id);
            lista_lru.pop_back();
        }

        // Agregar nuevo al frente
        lista_lru.push_front({page_id, datos});
        mapa[page_id] = lista_lru.begin();
    }

    void clear() {
        lista_lru.clear();
        mapa.clear();
    }
};

class Paginador {

    private:

    MapeoMemoria archivo;
    size_t num_paginas; // size_t es uint64_t (mayor rango que PageID que es uint32_t)
    std::set<PaginaID> paginas_libres; //IDs de paginas liberadas para reusarlas

    // Caché LRU con capacidad para 1024 páginas (4MB de caché con páginas de 4KB)
    CacheLRU cache;

    public:

    Paginador();
    ~Paginador();

    bool abrir(const std::string& ruta, size_t paginas_iniciales);
    void cerrar();

    PaginaID alloc_pagina();
    void liberar_pagina(PaginaID page_id);

    char* get_pagina(PaginaID page_id);
    size_t get_num_paginas() const;

};