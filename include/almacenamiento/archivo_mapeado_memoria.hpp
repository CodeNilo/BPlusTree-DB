#include <string>
#include <cstddef>
#include <windows.h>
#pragma once

class MapeoMemoria {

    private:

        // Windows y Linux tienen maneras diferentes de manejar archivos

        // Windows los maneja con HANDLES que es un puntero void* para rastrear a los archivos abiertos y en memoria
        #ifdef _WIN32
            HANDLE archivo_handle;
            HANDLE mapeo_handle;

        // Linux lo maneja con un entero llamado file descriptor. Es el estandar "POSIX"
        #else
            int archivo_fd;
        #endif

        char* datos;
        size_t size;

    public:

        MapeoMemoria();
        ~MapeoMemoria();

        bool abrir (const std::string& ruta, size_t initial_size);

        bool cerrar ();

        bool redimensionar (size_t nuevo_size);
        char* obtener_datos();

        size_t get_size () const;

};