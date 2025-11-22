#include "types.hpp"
#include <string>
#include <vector>
#include <cstring>

#pragma once

struct Ciudadano {
    DNI_t dni;
    std::string nombres;
    std::string apellidos;
    std::string direccion;

    Ciudadano(DNI_t dni, std::string nombres, std::string apellidos, std::string direccion)
        : dni(dni), nombres(std::move(nombres)), apellidos(std::move(apellidos)), direccion(std::move(direccion)) {}

    Ciudadano() = default;
};

inline size_t calcular_tamano_serializado(const Ciudadano& c) {
    return sizeof(DNI_t) +
           sizeof(uint16_t) + c.nombres.length() +
           sizeof(uint16_t) + c.apellidos.length() +
           sizeof(uint16_t) + c.direccion.length();
}

inline size_t serializar(const Ciudadano& c, char* buffer) {
    char* ptr = buffer;

    memcpy(ptr, &c.dni, sizeof(DNI_t));
    ptr += sizeof(DNI_t);

    uint16_t len_nombres = c.nombres.length();
    memcpy(ptr, &len_nombres, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    memcpy(ptr, c.nombres.c_str(), len_nombres);
    ptr += len_nombres;

    uint16_t len_apellidos = c.apellidos.length();
    memcpy(ptr, &len_apellidos, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    memcpy(ptr, c.apellidos.c_str(), len_apellidos);
    ptr += len_apellidos;

    uint16_t len_direccion = c.direccion.length();
    memcpy(ptr, &len_direccion, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    memcpy(ptr, c.direccion.c_str(), len_direccion);
    ptr += len_direccion;

    return ptr - buffer;
}

inline void deserializar(const char* buffer, size_t /*size*/, Ciudadano& out_c) {
    const char* ptr = buffer;

    memcpy(&out_c.dni, ptr, sizeof(DNI_t));
    ptr += sizeof(DNI_t);

    uint16_t len_nombres;
    memcpy(&len_nombres, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    out_c.nombres.assign(ptr, len_nombres);
    ptr += len_nombres;

    uint16_t len_apellidos;
    memcpy(&len_apellidos, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    out_c.apellidos.assign(ptr, len_apellidos);
    ptr += len_apellidos;

    uint16_t len_direccion;
    memcpy(&len_direccion, ptr, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    out_c.direccion.assign(ptr, len_direccion);
    ptr += len_direccion;
}

inline Ciudadano deserializar(const char* buffer, size_t size) {
    Ciudadano c;
    deserializar(buffer, size, c); // Llama a la otra versión para hacer el trabajo
    return c;
}
