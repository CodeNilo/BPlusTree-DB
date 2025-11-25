#include "almacenamiento/pagina.hpp"
#include <cstdint>
#include <cstddef>

#pragma once

using SlotID = uint16_t;
constexpr SlotID INVALID_SLOT_ID = 0xFFFF;

struct Slot {
    uint16_t offset;
    uint16_t size;
};

struct HeaderPaginaRanurada {
    HeaderPagina comun; // Esta en el byte 0
    uint16_t num_registros;
    uint16_t espacio_libre_inicio;
    uint16_t espacio_libre_fin;
    uint16_t flags;
};

class PaginaRanurada {

private:
    char* datos;

    HeaderPaginaRanurada* obtener_header();
    const HeaderPaginaRanurada* obtener_header() const;

    Slot* obtener_slot(SlotID slot_id);

public:
    const Slot* obtener_slot_const(SlotID slot_id) const;

    PaginaRanurada(char* pagina_bytes);

    void inicializar();

    SlotID insertar_registro(const char* registro, size_t size);
    bool insertar_registro_en_slot(SlotID slot_id, const char* registro, size_t size);
    bool leer_registro(SlotID slot_id, char* buffer, size_t& size);
    bool borrar_registro(SlotID slot_id);

    bool tiene_espacio(size_t size) const;
    uint16_t get_num_registros() const;
};