#include "almacenamiento/archivo_mapeado_memoria.hpp"
#include <windows.h>
#include <fileapi.h>


MapeoMemoria::MapeoMemoria() {

    archivo_handle = INVALID_HANDLE_VALUE;
    mapeo_handle = NULL;
    datos = nullptr;
    size = 0;

}

MapeoMemoria::~MapeoMemoria() {
    cerrar();
}

bool MapeoMemoria::abrir (const std::string& ruta, size_t initial_size) {

    /*
    La documentacion de microsoft nos dice que:

    HANDLE CreateFileA(
        [in]           LPCSTR                lpFileName,
        [in]           DWORD                 dwDesiredAccess,
        [in]           DWORD                 dwShareMode,
        [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        [in]           DWORD                 dwCreationDisposition,
        [in]           DWORD                 dwFlagsAndAttributes,
        [in, optional] HANDLE                hTemplateFile
    );
    */

    archivo_handle = CreateFileA(
        ruta.c_str(),                       // Nombre del archivo
        GENERIC_READ | GENERIC_WRITE,       // Queremos leer Y escribir
        0,                                  // No compartir (exclusivo)
        NULL,                               // Sin atributos de seguridad
        OPEN_ALWAYS,                        // Abre si existe, crea si no existe
        FILE_ATTRIBUTE_NORMAL,              // Archivo Normal
        NULL                                // Sin template
    );

    // En la documentacion se especifica que si CreateFileA falla devuelve INVALID_HANDLE_VALUE
    if (archivo_handle == INVALID_HANDLE_VALUE) { return false; }

    // Si initial_size es 0, usar el tamaño del archivo existente
    size_t tamano_mapeo = initial_size;
    if (tamano_mapeo == 0) {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(archivo_handle, &file_size)) {
            tamano_mapeo = static_cast<size_t>(file_size.QuadPart);
        }
        // Si el archivo está vacío y initial_size es 0, no podemos crear un mapeo válido
        if (tamano_mapeo == 0) {
            CloseHandle(archivo_handle);
            return false;
        }
    }

    /*
    Sacado de la documentacion de microsoft

    HANDLE CreateFileMappingA(
        [in]           HANDLE                hFile,
        [in, optional] LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
        [in]           DWORD                 flProtect,
        [in]           DWORD                 dwMaximumSizeHigh,
        [in]           DWORD                 dwMaximumSizeLow,
        [in, optional] LPCSTR                lpName
    );
    */

    // DWORD es un tipo de dato definido por Windows en la API de Win32.
    // WORD = 16 bits, DWORD = 32bits
    // DWORD = unsigned long con un rango de (0......4,294,967,295)
    mapeo_handle = CreateFileMappingA(
        archivo_handle,                    // Handle que creamos antes
        NULL,                              // Sin atributos de seguridad
        PAGE_READWRITE,                    // Poder leer y escribir
        // Algunas funciones de windows solo aceptan 64bits dividido en dos valores de 32 bits. High-order DWORD & Low-order DWORD
        (DWORD)(tamano_mapeo >> 32),       // Tamaño alto (64-bit)
        (DWORD)(tamano_mapeo & 0xFFFFFFFF), // Tamaño bajo (32-bit)
        NULL
        );

    // La documentacion dice que si falla CreateFileMappingA devuelve un NULL
    if (mapeo_handle == NULL) {
        CloseHandle(archivo_handle);
        return false;
    }

    /*
    LPVOID MapViewOfFile(
        [in] HANDLE hFileMappingObject,
        [in] DWORD  dwDesiredAccess,
        [in] DWORD  dwFileOffsetHigh,
        [in] DWORD  dwFileOffsetLow,
        [in] SIZE_T dwNumberOfBytesToMap
    );
    */

    datos = (char*)MapViewOfFile( // Asignamos directamente el valor a la variable global "datos" de la clase
        mapeo_handle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        tamano_mapeo
    );

    if (datos == nullptr) {
        CloseHandle(archivo_handle);
        CloseHandle(mapeo_handle);
        return false;
    }

    // Actualizamos las variables globales de la clase para guardar datos y tamano
    // si no cuando retornemos la funcion perdemos estos datos
    size = tamano_mapeo;

    return true;
}


bool MapeoMemoria::cerrar () {

    if (archivo_handle == INVALID_HANDLE_VALUE || mapeo_handle == NULL|| datos == nullptr) {
        return false;
    }

    UnmapViewOfFile(datos);
    CloseHandle(mapeo_handle);
    CloseHandle(archivo_handle);

    datos = nullptr;
    size = 0;
    archivo_handle = INVALID_HANDLE_VALUE;
    mapeo_handle = NULL;

    return true;
}

bool MapeoMemoria::redimensionar (size_t nuevo_size) {

    if (archivo_handle == INVALID_HANDLE_VALUE || mapeo_handle == NULL|| datos == nullptr) {
        return false;
    }

    if (nuevo_size == size) {
        return true;
    }

    HANDLE mapeo_viejo = mapeo_handle;
    char* datos_viejos = datos;
    // size_t size_viejo = size; // Unused variable

    HANDLE mapeo_nuevo = CreateFileMappingA(
        archivo_handle,
        NULL,
        PAGE_READWRITE,
        (DWORD)(nuevo_size >> 32),
        (DWORD)(nuevo_size & 0xFFFFFFFF),
        NULL
    );

    if (mapeo_nuevo == NULL) {
        return false; //Mantenemos el mapeo_handle antiguo
    }

    char* datos_nuevos = (char*)MapViewOfFile(
        mapeo_nuevo,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        nuevo_size
    );

    if (datos_nuevos == nullptr) {
        CloseHandle(mapeo_nuevo);
        return false;
    }

    // Cerramos el mapeo viejo
    UnmapViewOfFile(datos_viejos);
    CloseHandle(mapeo_viejo);

    mapeo_handle = mapeo_nuevo;
    datos = datos_nuevos;
    size = nuevo_size;

    return true;
}

char* MapeoMemoria::obtener_datos() {
    return datos;
}

size_t MapeoMemoria::get_size () const {
    return size;
}