#include "almacenamiento/pagina.hpp"
#include "almacenamiento/pagina_ranurada.hpp"
#include <cstdint>

#pragma once

using DNI_t = uint32_t;

struct RegistroID {
    PaginaID pagina_id;
    SlotID slot_id;

    bool operator==(const RegistroID& other) const {
        return pagina_id == other.pagina_id && slot_id == other.slot_id;
    }
};
