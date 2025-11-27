#pragma once

#include "almacenamiento/paginador.hpp"
#include "index/bplustree.hpp"
#include "core/ciudadano.hpp"
#include <string>
#include <stdexcept>
#include <optional>
#include <memory>

struct Superblock {
    PaginaID raiz_indice_dni;
    PaginaID ultima_pagina_datos;
};

constexpr PaginaID SUPERBLOCK_PAGE_ID = 0;

class Database {
public:
    Database();
    ~Database();

    bool abrir(const std::string& ruta);

    bool insertar_ciudadano(const Ciudadano& ciudadano);
    std::optional<Ciudadano> buscar_ciudadano(DNI_t dni);
    bool modificar_ciudadano(const Ciudadano& ciudadano);
    bool eliminar_ciudadano(DNI_t dni);

private:
    Paginador paginador;
    std::unique_ptr<BPlusTree> indice_dni;
    bool inicializado = false;
    std::string ruta_db;
    PaginaID ultima_pagina_datos_id;

    void crear_db(const std::string& ruta);
    void cargar_db();
    void cerrar();
};
