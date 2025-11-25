#include "almacenamiento/pagina_ranurada.hpp"
#include <cstring>

PaginaRanurada::PaginaRanurada(char* pagina_bytes) {
    datos = pagina_bytes; // Guardamos el puntero a los 4kb que nos dio el paginador
}

// Aca interpretamos los primeros 8 bytes de datos como un HeaderPaginaRanurada:
// datos[0..7] -> [num_registros][espacio_libre_inicio][espacio_libre_fin][flags]
HeaderPaginaRanurada* PaginaRanurada::obtener_header() {
    return reinterpret_cast<HeaderPaginaRanurada*>(datos); //reinterpret_cast le dice al compilador que trate los bytes como struct
}

const HeaderPaginaRanurada* PaginaRanurada::obtener_header() const {
    return reinterpret_cast<const HeaderPaginaRanurada*>(datos);
}

// Calculamos donde esta el slot que se esta pasando en la pagina, si el header ocupa 8 bytes
// tenemos que ver donde estarian los slots: por ejemplo el slot 0 en el byte 8, slot 1 byte 12 y asi..
Slot* PaginaRanurada::obtener_slot(SlotID slot_id) {
    size_t offset = sizeof(HeaderPaginaRanurada) + (slot_id * sizeof(Slot));
    return reinterpret_cast<Slot*>(datos + offset);
}

const Slot* PaginaRanurada::obtener_slot_const(SlotID slot_id) const {
    size_t offset = sizeof(HeaderPaginaRanurada) + (slot_id * sizeof(Slot));
    return reinterpret_cast<const Slot*>(datos + offset);
}

void PaginaRanurada::inicializar() {
    HeaderPaginaRanurada* header = obtener_header();

    header->num_registros = 0;
    header->espacio_libre_inicio = sizeof(HeaderPaginaRanurada);
    header->espacio_libre_fin = PAGINA_SIZE;
    header->flags = 0;
}

SlotID PaginaRanurada::insertar_registro(const char* registro, size_t size) {
    HeaderPaginaRanurada* header = obtener_header();

    size_t espacio_necesario_slot = sizeof(Slot);
    size_t espacio_necesario_datos = size;
    size_t espacio_total = espacio_necesario_slot + espacio_necesario_datos;

    if (!tiene_espacio(espacio_total)) {
        return INVALID_SLOT_ID;
    }

    SlotID nuevo_slot_id = header->num_registros;

    uint16_t offset_datos = header->espacio_libre_fin - size;

    memcpy(datos + offset_datos, registro, size);

    Slot* nuevo_slot = obtener_slot(nuevo_slot_id);
    nuevo_slot->offset = offset_datos;
    nuevo_slot->size = size;

    header->num_registros++;
    header->espacio_libre_inicio += sizeof(Slot);
    header->espacio_libre_fin = offset_datos;

    return nuevo_slot_id;
}

bool PaginaRanurada::insertar_registro_en_slot(SlotID slot_id, const char* registro, size_t size) {
    HeaderPaginaRanurada* header = obtener_header();

    if (slot_id >= header->num_registros) {
        return false; // Slot no válido
    }

    if (!tiene_espacio(size)) {
        return false;
    }

    Slot* slot = obtener_slot(slot_id);
    if (slot->size != 0) {
        return false; // El slot debe estar vacío (previamente borrado)
    }

    uint16_t offset_datos = header->espacio_libre_fin - size;
    memcpy(datos + offset_datos, registro, size);

    slot->offset = offset_datos;
    slot->size = size;
    header->espacio_libre_fin = offset_datos;
    return true;
}

bool PaginaRanurada::leer_registro(SlotID slot_id, char* buffer, size_t& size) {
    HeaderPaginaRanurada* header = obtener_header();

    if (slot_id >= header->num_registros) {
        return false;
    }

    Slot* slot = obtener_slot(slot_id);

    if (slot->size == 0) {
        return false;
    }

    memcpy(buffer, datos + slot->offset, slot->size);
    size = slot->size;

    return true;
}

bool PaginaRanurada::borrar_registro(SlotID slot_id) {
    HeaderPaginaRanurada* header = obtener_header();

    if (slot_id >= header->num_registros) {
        return false;
    }

    Slot* slot = obtener_slot(slot_id);

    if (slot->size == 0) {
        return false;
    }

    slot->size = 0;
    slot->offset = 0;

    return true;
}

bool PaginaRanurada::tiene_espacio(size_t size) const {
    const HeaderPaginaRanurada* header = obtener_header();

    size_t espacio_libre = header->espacio_libre_fin - header->espacio_libre_inicio;

    return espacio_libre >= size;
}

uint16_t PaginaRanurada::get_num_registros() const {
    const HeaderPaginaRanurada* header = obtener_header();
    return header->num_registros;
}
