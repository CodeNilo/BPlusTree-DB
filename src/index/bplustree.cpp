#include "index/bplustree.hpp"
#include <algorithm>
#include <stdexcept>

BPlusTree::BPlusTree(Paginador& paginador)
    : paginador(paginador), id_raiz(INVALID_PAGE_ID) {}

PaginaID BPlusTree::inicializar(PaginaID id_raiz) {
    this->id_raiz = id_raiz;
    if (this->id_raiz == INVALID_PAGE_ID) {
        PaginaID nueva_raiz_id = paginador.alloc_pagina();
        if (nueva_raiz_id == INVALID_PAGE_ID) {
            throw std::runtime_error("No se pudo asignar una pagina para la nueva raiz del B+ Tree.");
        }
        this->id_raiz = nueva_raiz_id;

        char* pagina_ptr = paginador.get_pagina(this->id_raiz);
        auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);
        header->tipo = TipoNodo::Hoja;
        header->num_claves = 0;

        PaginaID* sig_hoja_ptr = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));
        *sig_hoja_ptr = INVALID_PAGE_ID;
    }
    return this->id_raiz;
}

PaginaID BPlusTree::get_id_raiz() const {
    return this->id_raiz;
}

PaginaID BPlusTree::buscar_hoja(DNI_t clave) {
    PaginaID id_pagina_actual = id_raiz;
    while (true) {
        char* pagina_ptr = paginador.get_pagina(id_pagina_actual);
        auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);

        if (header->tipo == TipoNodo::Hoja) {
            return id_pagina_actual;
        }

        auto claves = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
        auto hijos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));

        auto it = std::upper_bound(claves, claves + header->num_claves, clave);
        size_t pos = std::distance(claves, it);
        id_pagina_actual = hijos[pos];
    }
}

std::optional<RegistroID> BPlusTree::buscar(DNI_t clave) {
    PaginaID id_hoja = buscar_hoja(clave);
    char* pagina_ptr = paginador.get_pagina(id_hoja);

    auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);
    auto entradas = reinterpret_cast<Hoja::Entrada*>(pagina_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));

    auto it = std::lower_bound(entradas, entradas + header->num_claves, clave,
        [](const Hoja::Entrada& a, DNI_t b) {
            return a.clave < b;
        });

    if (it != entradas + header->num_claves && it->clave == clave) {
        return it->valor;
    }

    return std::nullopt;
}

bool BPlusTree::insertar(DNI_t clave, RegistroID valor) {
    if (id_raiz == INVALID_PAGE_ID) {
        this->id_raiz = inicializar(INVALID_PAGE_ID);
    }

    auto resultado = insertar_en_nodo(id_raiz, clave, valor);

    if (resultado.has_value()) {
        PaginaID nueva_raiz_id = paginador.alloc_pagina();
        if (nueva_raiz_id == INVALID_PAGE_ID) {
            return false;
        }
        char* nueva_raiz_ptr = paginador.get_pagina(nueva_raiz_id);
        auto header = reinterpret_cast<BPlusTreeHeader*>(nueva_raiz_ptr);
        header->tipo = TipoNodo::Interno;
        header->num_claves = 1;

        auto hijos = reinterpret_cast<PaginaID*>(nueva_raiz_ptr + sizeof(BPlusTreeHeader));
        auto claves = reinterpret_cast<DNI_t*>(nueva_raiz_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));

        hijos[0] = id_raiz;
        claves[0] = resultado->clave_promocionada;
        hijos[1] = resultado->id_nueva_pagina;

        id_raiz = nueva_raiz_id;
    }

    return true;
}

std::optional<BPlusTree::ResultadoDivision> BPlusTree::insertar_en_nodo(PaginaID id_pagina, DNI_t clave, RegistroID valor) {
    char* pagina_ptr = paginador.get_pagina(id_pagina);
    auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);

    if (header->tipo == TipoNodo::Hoja) {
        insertar_en_hoja(pagina_ptr, clave, valor);

        if (header->num_claves > Hoja::MAX_CLAVES) {
            PaginaID nueva_hoja_id = paginador.alloc_pagina();
            if (nueva_hoja_id == INVALID_PAGE_ID) {
                throw std::runtime_error("No se pudo asignar una nueva pagina para la division de hoja.");
            }

            // El remapeo invalida punteros previos, refrescamos antes de usarlos
            pagina_ptr = paginador.get_pagina(id_pagina);
            header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);

            char* nueva_pagina_ptr = paginador.get_pagina(nueva_hoja_id);
            auto nuevo_header = reinterpret_cast<BPlusTreeHeader*>(nueva_pagina_ptr);
            nuevo_header->tipo = TipoNodo::Hoja;

            auto entradas = reinterpret_cast<Hoja::Entrada*>(pagina_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));
            auto nuevas_entradas = reinterpret_cast<Hoja::Entrada*>(nueva_pagina_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));

            size_t punto_medio = header->num_claves / 2;
            DNI_t clave_promocionada = entradas[punto_medio].clave;

            std::copy(entradas + punto_medio, entradas + header->num_claves, nuevas_entradas);
            nuevo_header->num_claves = header->num_claves - punto_medio;
            header->num_claves = punto_medio;

            auto sig_ptr = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));
            auto nuevo_sig_ptr = reinterpret_cast<PaginaID*>(nueva_pagina_ptr + sizeof(BPlusTreeHeader));
            *nuevo_sig_ptr = *sig_ptr;
            *sig_ptr = nueva_hoja_id;

            return ResultadoDivision{clave_promocionada, nueva_hoja_id};
        }
        return std::nullopt;
    }

    // NODO INTERNO
    auto claves = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));

    auto it = std::lower_bound(claves, claves + header->num_claves, clave);
    size_t pos = std::distance(claves, it);
    PaginaID id_hijo;

    if (it != claves + header->num_claves && *it == clave) {
        id_hijo = hijos[pos + 1];
    } else {
        id_hijo = hijos[pos];
    }

    auto resultado_division = insertar_en_nodo(id_hijo, clave, valor);

    if (resultado_division.has_value()) {
        // El remapeo pudo ocurrir en el subarbol, refrescamos punteros antes de usarlos
        pagina_ptr = paginador.get_pagina(id_pagina);
        header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);
        claves = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
        hijos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));

        insertar_en_interno(pagina_ptr, resultado_division->clave_promocionada, resultado_division->id_nueva_pagina);

        if (header->num_claves > Interno::MAX_CLAVES) {
            PaginaID nueva_pagina_id = paginador.alloc_pagina();
            if (nueva_pagina_id == INVALID_PAGE_ID) {
                throw std::runtime_error("No se pudo asignar una nueva pagina para la division de nodo interno.");
            }

            // El remapeo invalida punteros previos, refrescamos antes de usarlos
            pagina_ptr = paginador.get_pagina(id_pagina);
            header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);
            claves = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
            hijos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));

            char* nueva_pagina_ptr = paginador.get_pagina(nueva_pagina_id);
            auto nuevo_header = reinterpret_cast<BPlusTreeHeader*>(nueva_pagina_ptr);
            nuevo_header->tipo = TipoNodo::Interno;

            auto claves_viejas = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
            auto hijos_viejos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));
            auto nuevas_claves = reinterpret_cast<DNI_t*>(nueva_pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
            auto nuevos_hijos = reinterpret_cast<PaginaID*>(nueva_pagina_ptr + sizeof(BPlusTreeHeader));

            size_t punto_medio_idx = header->num_claves / 2;
            DNI_t clave_promocionada = claves_viejas[punto_medio_idx];

            std::copy(claves_viejas + punto_medio_idx + 1, claves_viejas + header->num_claves, nuevas_claves);
            std::copy(hijos_viejos + punto_medio_idx + 1, hijos_viejos + header->num_claves + 1, nuevos_hijos);

            nuevo_header->num_claves = header->num_claves - punto_medio_idx - 1;
            header->num_claves = punto_medio_idx;

            return ResultadoDivision{clave_promocionada, nueva_pagina_id};
        }
    }

    return std::nullopt;
}

void BPlusTree::insertar_en_hoja(char* pagina_ptr, DNI_t clave, RegistroID valor) {
    auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);
    auto entradas = reinterpret_cast<Hoja::Entrada*>(pagina_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));

    auto it = std::lower_bound(entradas, entradas + header->num_claves, clave,
        [](const Hoja::Entrada& a, DNI_t b) {
            return a.clave < b;
        });

    size_t pos = std::distance(entradas, it);

    std::move_backward(entradas + pos, entradas + header->num_claves, entradas + header->num_claves + 1);

    entradas[pos] = {clave, valor};
    header->num_claves++;
}

void BPlusTree::insertar_en_interno(char* pagina_ptr, DNI_t clave, PaginaID id_hijo_derecho) {
    auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);
    auto claves = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));

    auto it = std::lower_bound(claves, claves + header->num_claves, clave);
    size_t pos = std::distance(claves, it);

    std::move_backward(claves + pos, claves + header->num_claves, claves + header->num_claves + 1);
    std::move_backward(hijos + pos + 1, hijos + header->num_claves + 1, hijos + header->num_claves + 2);

    claves[pos] = clave;
    hijos[pos + 1] = id_hijo_derecho;
    header->num_claves++;
}

bool BPlusTree::eliminar(DNI_t clave) {
    if (id_raiz == INVALID_PAGE_ID) {
        return false;
    }
    eliminar_interno(id_raiz, clave, INVALID_PAGE_ID, -1);

    // Si la raiz queda vacia despues de una fusion, la eliminamos
    // y la nueva raiz es su unico hijo.
    char* raiz_ptr = paginador.get_pagina(id_raiz);
    auto header_raiz = reinterpret_cast<BPlusTreeHeader*>(raiz_ptr);
    if (header_raiz->tipo == TipoNodo::Interno && header_raiz->num_claves == 0) {
        auto hijos_raiz = reinterpret_cast<PaginaID*>(raiz_ptr + sizeof(BPlusTreeHeader));
        PaginaID nueva_raiz_id = hijos_raiz[0];
        paginador.liberar_pagina(id_raiz);
        id_raiz = nueva_raiz_id;
    }

    return true;
}

void BPlusTree::eliminar_interno(PaginaID id_pagina, DNI_t clave, PaginaID id_padre, int indice_en_padre) {
    char* pagina_ptr = paginador.get_pagina(id_pagina);
    auto header = reinterpret_cast<BPlusTreeHeader*>(pagina_ptr);

    if (header->tipo == TipoNodo::Hoja) {
        auto entradas = reinterpret_cast<Hoja::Entrada*>(pagina_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));
        auto it = std::lower_bound(entradas, entradas + header->num_claves, clave, [](const Hoja::Entrada& a, DNI_t b) { return a.clave < b; });

        if (it == entradas + header->num_claves || it->clave != clave) {
            return; // La clave no existe
        }

        size_t pos = std::distance(entradas, it);
        std::move(entradas + pos + 1, entradas + header->num_claves, entradas + pos);
        header->num_claves--;

    } else { // Nodo Interno
        auto claves = reinterpret_cast<DNI_t*>(pagina_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
        auto hijos = reinterpret_cast<PaginaID*>(pagina_ptr + sizeof(BPlusTreeHeader));
        auto it = std::upper_bound(claves, claves + header->num_claves, clave);
        size_t pos = std::distance(claves, it);
        eliminar_interno(hijos[pos], clave, id_pagina, static_cast<int>(pos));
    }

    // Verificar underflow (solo si no es la raiz)
    size_t min_claves = (header->tipo == TipoNodo::Hoja) ? (Hoja::MAX_CLAVES / 2) : (Interno::MAX_CLAVES / 2);
    if (header->num_claves >= min_claves || id_padre == INVALID_PAGE_ID) {
        return;
    }

    // Manejar underflow
    auto info_hermano_opt = buscar_hermano(id_padre, indice_en_padre);
    if (!info_hermano_opt.has_value()) return; // No deberia pasar si no es la raiz

    auto info_hermano = info_hermano_opt.value();
    char* padre_ptr = paginador.get_pagina(id_padre);
    char* hermano_ptr = paginador.get_pagina(info_hermano.id);
    auto header_hermano = reinterpret_cast<BPlusTreeHeader*>(hermano_ptr);

    size_t min_hermano = (header_hermano->tipo == TipoNodo::Hoja) ? (Hoja::MAX_CLAVES / 2) : (Interno::MAX_CLAVES / 2);

    if (header_hermano->num_claves > min_hermano) {
        // Redistribucion
        if (header->tipo == TipoNodo::Hoja) {
            redistribuir_hojas(hermano_ptr, pagina_ptr, info_hermano.direccion, padre_ptr, info_hermano.indice_en_padre);
        } else {
            redistribuir_internos(hermano_ptr, pagina_ptr, info_hermano.direccion, padre_ptr, info_hermano.indice_en_padre);
        }
    } else {
        // Fusion
        if (header->tipo == TipoNodo::Hoja) {
            fusionar_hojas(hermano_ptr, pagina_ptr, info_hermano.direccion, padre_ptr, info_hermano.indice_en_padre);
            paginador.liberar_pagina(info_hermano.direccion == DireccionHermano::Derecho ? info_hermano.id : id_pagina);
        } else {
            fusionar_internos(hermano_ptr, pagina_ptr, info_hermano.direccion, padre_ptr, info_hermano.indice_en_padre);
            paginador.liberar_pagina(info_hermano.direccion == DireccionHermano::Derecho ? info_hermano.id : id_pagina);
        }
    }
}

std::optional<BPlusTree::InfoHermano> BPlusTree::buscar_hermano(PaginaID id_padre, int indice_en_padre) {
    char* padre_ptr = paginador.get_pagina(id_padre);
    auto header_padre = reinterpret_cast<BPlusTreeHeader*>(padre_ptr);
    auto hijos = reinterpret_cast<PaginaID*>(padre_ptr + sizeof(BPlusTreeHeader));

    if (indice_en_padre > 0) { // Intentar con hermano izquierdo
        return InfoHermano{hijos[indice_en_padre - 1], indice_en_padre - 1, DireccionHermano::Izquierdo};
    }
    if (indice_en_padre < header_padre->num_claves) { // Intentar con hermano derecho
        return InfoHermano{hijos[indice_en_padre + 1], indice_en_padre, DireccionHermano::Derecho};
    }
    return std::nullopt;
}

void BPlusTree::redistribuir_hojas(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre) {
    auto header_actual = reinterpret_cast<BPlusTreeHeader*>(pagina_actual_ptr);
    auto header_hermano = reinterpret_cast<BPlusTreeHeader*>(pagina_hermano_ptr);
    auto entradas_actuales = reinterpret_cast<Hoja::Entrada*>(pagina_actual_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));
    auto entradas_hermano = reinterpret_cast<Hoja::Entrada*>(pagina_hermano_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));
    auto claves_padre = reinterpret_cast<DNI_t*>(pagina_padre_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));

    if (direccion == DireccionHermano::Izquierdo) { // Hermano a la izquierda
        // Mover la ultima entrada del hermano al principio del nodo actual
        Hoja::Entrada& entrada_prestada = entradas_hermano[header_hermano->num_claves - 1];
        std::move_backward(entradas_actuales, entradas_actuales + header_actual->num_claves, entradas_actuales + header_actual->num_claves + 1);
        entradas_actuales[0] = entrada_prestada;
        header_hermano->num_claves--;
        header_actual->num_claves++;
        // Actualizar clave en el padre
        claves_padre[indice_padre] = entradas_actuales[0].clave;
    } else { // Hermano a la derecha
        // Mover la primera entrada del hermano al final del nodo actual
        Hoja::Entrada& entrada_prestada = entradas_hermano[0];
        entradas_actuales[header_actual->num_claves] = entrada_prestada;
        std::move(entradas_hermano + 1, entradas_hermano + header_hermano->num_claves, entradas_hermano);
        header_hermano->num_claves--;
        header_actual->num_claves++;
        // Actualizar clave en el padre
        claves_padre[indice_padre] = entradas_hermano[0].clave;
    }
}

void BPlusTree::fusionar_hojas(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre) {
    char* nodo_izq_ptr = (direccion == DireccionHermano::Izquierdo) ? pagina_hermano_ptr : pagina_actual_ptr;
    char* nodo_der_ptr = (direccion == DireccionHermano::Izquierdo) ? pagina_actual_ptr : pagina_hermano_ptr;

    auto header_izq = reinterpret_cast<BPlusTreeHeader*>(nodo_izq_ptr);
    auto header_der = reinterpret_cast<BPlusTreeHeader*>(nodo_der_ptr);
    auto entradas_izq = reinterpret_cast<Hoja::Entrada*>(nodo_izq_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));
    auto entradas_der = reinterpret_cast<Hoja::Entrada*>(nodo_der_ptr + sizeof(BPlusTreeHeader) + sizeof(PaginaID));

    // Copiar todas las entradas del nodo derecho al final del nodo izquierdo
    std::copy(entradas_der, entradas_der + header_der->num_claves, entradas_izq + header_izq->num_claves);
    header_izq->num_claves += header_der->num_claves;

    // Actualizar puntero de la lista enlazada de hojas
    auto sig_ptr_izq = reinterpret_cast<PaginaID*>(nodo_izq_ptr + sizeof(BPlusTreeHeader));
    auto sig_ptr_der = reinterpret_cast<PaginaID*>(nodo_der_ptr + sizeof(BPlusTreeHeader));
    *sig_ptr_izq = *sig_ptr_der;

    // Eliminar la clave y el puntero del padre
    auto claves_padre = reinterpret_cast<DNI_t*>(pagina_padre_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos_padre = reinterpret_cast<PaginaID*>(pagina_padre_ptr + sizeof(BPlusTreeHeader));
    auto header_padre = reinterpret_cast<BPlusTreeHeader*>(pagina_padre_ptr);

    std::move(claves_padre + indice_padre + 1, claves_padre + header_padre->num_claves, claves_padre + indice_padre);
    std::move(hijos_padre + indice_padre + 2, hijos_padre + header_padre->num_claves + 1, hijos_padre + indice_padre + 1);
    header_padre->num_claves--;
}

void BPlusTree::redistribuir_internos(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre) {
    auto header_actual = reinterpret_cast<BPlusTreeHeader*>(pagina_actual_ptr);
    auto header_hermano = reinterpret_cast<BPlusTreeHeader*>(pagina_hermano_ptr);
    auto claves_actuales = reinterpret_cast<DNI_t*>(pagina_actual_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos_actuales = reinterpret_cast<PaginaID*>(pagina_actual_ptr + sizeof(BPlusTreeHeader));
    auto claves_hermano = reinterpret_cast<DNI_t*>(pagina_hermano_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos_hermano = reinterpret_cast<PaginaID*>(pagina_hermano_ptr + sizeof(BPlusTreeHeader));
    auto claves_padre = reinterpret_cast<DNI_t*>(pagina_padre_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));

    if (direccion == DireccionHermano::Izquierdo) {
        // Mover clave del padre al inicio del nodo actual
        std::move_backward(claves_actuales, claves_actuales + header_actual->num_claves, claves_actuales + header_actual->num_claves + 1);
        claves_actuales[0] = claves_padre[indice_padre];

        // Mover puntero del hermano al nodo actual
        std::move_backward(hijos_actuales, hijos_actuales + header_actual->num_claves + 1, hijos_actuales + header_actual->num_claves + 2);
        hijos_actuales[0] = hijos_hermano[header_hermano->num_claves];

        // Actualizar clave del padre con la clave movida del hermano
        claves_padre[indice_padre] = claves_hermano[header_hermano->num_claves - 1];

        header_actual->num_claves++;
        header_hermano->num_claves--;
    } else { // Hermano a la derecha
        // Mover clave del padre al final del nodo actual
        claves_actuales[header_actual->num_claves] = claves_padre[indice_padre];

        // Mover puntero del hermano al nodo actual
        hijos_actuales[header_actual->num_claves + 1] = hijos_hermano[0];

        // Actualizar clave del padre con la clave movida del hermano
        claves_padre[indice_padre] = claves_hermano[0];

        // Compactar hermano
        std::move(claves_hermano + 1, claves_hermano + header_hermano->num_claves, claves_hermano);
        std::move(hijos_hermano + 1, hijos_hermano + header_hermano->num_claves + 1, hijos_hermano);

        header_actual->num_claves++;
        header_hermano->num_claves--;
    }
}

void BPlusTree::fusionar_internos(char* pagina_hermano_ptr, char* pagina_actual_ptr, DireccionHermano direccion, char* pagina_padre_ptr, int indice_padre) {
    char* nodo_izq_ptr = (direccion == DireccionHermano::Izquierdo) ? pagina_hermano_ptr : pagina_actual_ptr;
    char* nodo_der_ptr = (direccion == DireccionHermano::Izquierdo) ? pagina_actual_ptr : pagina_hermano_ptr;

    auto header_izq = reinterpret_cast<BPlusTreeHeader*>(nodo_izq_ptr);
    auto header_der = reinterpret_cast<BPlusTreeHeader*>(nodo_der_ptr);
    auto claves_izq = reinterpret_cast<DNI_t*>(nodo_izq_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos_izq = reinterpret_cast<PaginaID*>(nodo_izq_ptr + sizeof(BPlusTreeHeader));
    auto claves_der = reinterpret_cast<DNI_t*>(nodo_der_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos_der = reinterpret_cast<PaginaID*>(nodo_der_ptr + sizeof(BPlusTreeHeader));
    auto claves_padre = reinterpret_cast<DNI_t*>(pagina_padre_ptr + sizeof(BPlusTreeHeader) + Interno::ORDEN * sizeof(PaginaID));
    auto hijos_padre = reinterpret_cast<PaginaID*>(pagina_padre_ptr + sizeof(BPlusTreeHeader));
    auto header_padre = reinterpret_cast<BPlusTreeHeader*>(pagina_padre_ptr);

    // Bajar la clave del padre al final del nodo izquierdo
    claves_izq[header_izq->num_claves] = claves_padre[indice_padre];
    header_izq->num_claves++;

    // Copiar claves e hijos del nodo derecho al nodo izquierdo
    std::copy(claves_der, claves_der + header_der->num_claves, claves_izq + header_izq->num_claves);
    std::copy(hijos_der, hijos_der + header_der->num_claves + 1, hijos_izq + header_izq->num_claves);
    header_izq->num_claves += header_der->num_claves;

    // Eliminar clave y puntero del padre
    std::move(claves_padre + indice_padre + 1, claves_padre + header_padre->num_claves, claves_padre + indice_padre);
    std::move(hijos_padre + indice_padre + 2, hijos_padre + header_padre->num_claves + 1, hijos_padre + indice_padre + 1);
    header_padre->num_claves--;
}
