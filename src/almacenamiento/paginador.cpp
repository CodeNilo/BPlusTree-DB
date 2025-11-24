#include "almacenamiento/paginador.hpp"

Paginador::Paginador() : cache(1024) {
    num_paginas = 0;
}

Paginador::~Paginador() {
    cerrar();
}

bool Paginador::abrir(const std::string& ruta, size_t paginas_iniciales) {
    size_t bytes_necesarios = paginas_iniciales * PAGINA_SIZE;

    bool exito = archivo.abrir(ruta, bytes_necesarios);

    if (!exito) {
        return false;
    }

    // Al abrir un archivo existente, el tamaño real puede ser mayor.
    // Calculamos el número de páginas basado en el tamaño real del archivo.
    num_paginas = archivo.get_size() / PAGINA_SIZE;
    paginas_libres.clear();

    return true;
}

void Paginador::cerrar() {
    archivo.cerrar();
    num_paginas = 0;
    paginas_libres.clear();
    cache.clear();
}

PaginaID Paginador::alloc_pagina() {

    if (!paginas_libres.empty()) {
        PaginaID id = *paginas_libres.begin(); // begin() nos da un iterador al principio del set, pero queremos el valor entonces lo dereferenciamos
        paginas_libres.erase(id);
        return id;
    }

    if (num_paginas >= INVALID_PAGE_ID) {
        return INVALID_PAGE_ID;
    }

    // Se tuvo que poner num_paginas + 1 porque si hacemos num_paginas * PAGINA_SIZE
    // nos devuelve una cantidad justo al limite de todos los bytes posibles
    // entonces si hicieramos 10 * 4096 = 40,960 si se intenta acceder al byte 40,960
    // habra segmentation fault porque solo hay de 0 a 40,959 bytes
    size_t bytes_necesarios = (num_paginas + 1) * PAGINA_SIZE;

    if (archivo.get_size() < bytes_necesarios) {
        bool exito = archivo.redimensionar(bytes_necesarios);

        if (!exito) {
            return INVALID_PAGE_ID;
        }
        // El remapeo invalida punteros cacheados, asi que limpiamos la cache
        cache.clear();
    }

    return num_paginas++;
}

void Paginador::liberar_pagina(PaginaID page_id) {
    if (page_id >= num_paginas || page_id == INVALID_PAGE_ID) {
        return;
    }

    // No borramos ni liberamos recursos porque en realidad queremos reutilizar el espacio
    // entonces solo lo marcamos como libre
    paginas_libres.insert(page_id);
}

char* Paginador::get_pagina(PaginaID page_id) {
    if (page_id >= num_paginas || page_id == INVALID_PAGE_ID) {
        return nullptr;
    }

    // Primero verificar en caché
    char* cached = cache.get(page_id);
    if (cached != nullptr) {
        return cached;
    }

    // PAGINA SIZE = 4096 bytes entonces 10 x 4096 = 40960
    // Sabemos que empezando desde ese offset esta la informacion perteneciente a esa pagina
    size_t offset = page_id * PAGINA_SIZE;

    // obtener_datos retorna un puntero al byte 0 de datos, datos es un arreglo de bytes
    // usando el offset accedemos al espacio de memoria en donde esta la informacion perteneciente a esta pagina
    char* pagina = archivo.obtener_datos() + offset;

    // Agregar al caché
    cache.put(page_id, pagina);

    return pagina;
}

size_t Paginador::get_num_paginas() const {
    return num_paginas;
}
