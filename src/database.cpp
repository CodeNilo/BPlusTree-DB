#include "database.hpp"
#include "almacenamiento/pagina_ranurada.hpp"
#include "core/ciudadano.hpp"
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

Database::Database() : inicializado(false), ultima_pagina_datos_id(INVALID_PAGE_ID) {}

Database::~Database() {
    if (inicializado) {
        cerrar();
    }
}

bool Database::abrir(const std::string& ruta) {
    if (inicializado) {
        return false; // Ya está abierta
    }

    bool db_existe = fs::exists(ruta);

    if (!db_existe) {
        crear_db(ruta);
    } else {
        if (!paginador.abrir(ruta, 0)) {
            throw std::runtime_error("No se pudo abrir el archivo de la base de datos existente.");
        }
        cargar_db();
    }

    this->ruta_db = ruta;
    inicializado = true;
    return true;
}

void Database::cerrar() {
    if (!inicializado) {
        return;
    }
    char* superblock_ptr = paginador.get_pagina(SUPERBLOCK_PAGE_ID);
    auto* superblock = reinterpret_cast<Superblock*>(superblock_ptr);
    superblock->raiz_indice_dni = indice_dni->get_id_raiz();
    superblock->ultima_pagina_datos = ultima_pagina_datos_id;
    
    paginador.cerrar();
    indice_dni.reset();
    inicializado = false;
}

void Database::crear_db(const std::string& ruta) {
    if (!paginador.abrir(ruta, 10)) {
        throw std::runtime_error("No se pudo crear el archivo de la base de datos.");
    }

    indice_dni = std::make_unique<BPlusTree>(paginador);
    PaginaID raiz_id = indice_dni->inicializar(INVALID_PAGE_ID);

    char* superblock_ptr = paginador.get_pagina(SUPERBLOCK_PAGE_ID);
    auto* superblock = reinterpret_cast<Superblock*>(superblock_ptr);

    superblock->raiz_indice_dni = raiz_id;
    superblock->ultima_pagina_datos = INVALID_PAGE_ID;
    ultima_pagina_datos_id = INVALID_PAGE_ID;
}

void Database::cargar_db() {
    char* superblock_ptr = paginador.get_pagina(SUPERBLOCK_PAGE_ID);
    const auto* superblock = reinterpret_cast<const Superblock*>(superblock_ptr);

    indice_dni = std::make_unique<BPlusTree>(paginador);
    indice_dni->inicializar(superblock->raiz_indice_dni);
    ultima_pagina_datos_id = superblock->ultima_pagina_datos;
}

bool Database::insertar_ciudadano(const Ciudadano& ciudadano) {
    if (!inicializado) {
        return false;
    }

    if (indice_dni->buscar(ciudadano.dni).has_value()) {
        return false; // Ya existe, no se permiten duplicados por ahora
    }

    std::vector<char> buffer(PAGINA_SIZE);
    size_t size_serializado = serializar(ciudadano, buffer.data());

    PaginaID pagina_datos_id = INVALID_PAGE_ID;

    // Intentar insertar en la última página de datos conocida.
    if (ultima_pagina_datos_id != INVALID_PAGE_ID) {
        char* pagina_ptr = paginador.get_pagina(ultima_pagina_datos_id);
        PaginaRanurada pagina_ranurada(pagina_ptr);
        if (pagina_ranurada.tiene_espacio(sizeof(Slot) + size_serializado)) {
            pagina_datos_id = ultima_pagina_datos_id;
        }
    }

    // Si no hay última página o si está llena, asignamos una nueva.
    if (pagina_datos_id == INVALID_PAGE_ID) {
        pagina_datos_id = paginador.alloc_pagina();
        if (pagina_datos_id == INVALID_PAGE_ID) {
            return false;
        }
        char* nueva_pagina_ptr = paginador.get_pagina(pagina_datos_id);
        PaginaRanurada pagina_nueva(nueva_pagina_ptr);
        pagina_nueva.inicializar();
        ultima_pagina_datos_id = pagina_datos_id;
    }

    char* pagina_ptr = paginador.get_pagina(pagina_datos_id);
    PaginaRanurada pagina_ranurada(pagina_ptr);

    SlotID slot_id = pagina_ranurada.insertar_registro(buffer.data(), size_serializado);
    if (slot_id == INVALID_SLOT_ID) {
        return false;
    }

    RegistroID rid = {pagina_datos_id, slot_id};
    return indice_dni->insertar(ciudadano.dni, rid);
}

std::optional<Ciudadano> Database::buscar_ciudadano(DNI_t dni) {
    if (!inicializado) return std::nullopt;

    auto rid_optional = indice_dni->buscar(dni);
    if (!rid_optional.has_value()) {
        return std::nullopt;
    }

    RegistroID rid = rid_optional.value();

    char* pagina_ptr = paginador.get_pagina(rid.pagina_id);
    PaginaRanurada pagina_ranurada(pagina_ptr);

    std::vector<char> buffer(PAGINA_SIZE);
    size_t size_leido = 0;

    if (pagina_ranurada.leer_registro(rid.slot_id, buffer.data(), size_leido)) {
        return deserializar(buffer.data(), size_leido);
    }

    return std::nullopt;
}

bool Database::modificar_ciudadano(const Ciudadano& ciudadano) {
    if (!inicializado) return false;

    auto rid_optional = indice_dni->buscar(ciudadano.dni);
    if (!rid_optional.has_value()) {
        return false; // No se puede modificar un ciudadano que no existe.
    }

    RegistroID rid = rid_optional.value();

    // Serializar el nuevo ciudadano para saber su tamaño
    std::vector<char> buffer_nuevo(PAGINA_SIZE);
    size_t nuevo_size = serializar(ciudadano, buffer_nuevo.data());

    char* pagina_ptr = paginador.get_pagina(rid.pagina_id);
    PaginaRanurada pagina_ranurada(pagina_ptr);

    // Obtener el tamaño del registro antiguo
    const Slot* slot_antiguo = pagina_ranurada.obtener_slot_const(rid.slot_id);
    if (!slot_antiguo) return false;
    size_t antiguo_size = slot_antiguo->size;

    // Versión simple: solo permitimos modificar si el nuevo tamaño es <= al antiguo.
    if (nuevo_size > antiguo_size) {
        // TODO: Implementar la lógica para manejar registros que crecen.
        return false;
    }

    // Borrar el registro de datos y luego insertarlo de nuevo en el mismo slot.
    if (!pagina_ranurada.borrar_registro(rid.slot_id)) {
        return false;
    }
    return pagina_ranurada.insertar_registro_en_slot(rid.slot_id, buffer_nuevo.data(), nuevo_size);
}

bool Database::eliminar_ciudadano(DNI_t dni) {
    if (!inicializado) return false;

    auto rid_optional = indice_dni->buscar(dni);
    if (!rid_optional.has_value()) {
        return false; 
    }
    RegistroID rid = rid_optional.value();

    char* pagina_ptr = paginador.get_pagina(rid.pagina_id);
    if (!pagina_ptr) {
        return false;
    }
    PaginaRanurada pagina_ranurada(pagina_ptr);
    if (!pagina_ranurada.borrar_registro(rid.slot_id)) {
        return false;
    }

    // Finalmente, eliminar la clave del índice
    return indice_dni->eliminar(dni);
}
