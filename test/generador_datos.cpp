#include "generador_datos.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <string>

// Generador de numeros aleatorios
static std::random_device rd;
static std::mt19937 gen(rd());

// Listas de nombres y apellidos peruanos comunes
static const std::vector<std::string> nombres = {
    "Juan", "Maria", "Carlos", "Ana", "Luis", "Rosa", "Jorge", "Carmen",
    "Pedro", "Lucia", "Miguel", "Sofia", "Jose", "Isabel", "Ricardo",
    "Elena", "Fernando", "Patricia", "Roberto", "Teresa", "Alberto", "Laura",
    "Manuel", "Gloria", "Antonio", "Marta", "Francisco", "Diana", "Diego", "Sandra"
};

static const std::vector<std::string> apellidos = {
    "Garcia", "Rodriguez", "Martinez", "Fernandez", "Lopez", "Gonzalez",
    "Sanchez", "Perez", "Gomez", "Torres", "Ramirez", "Flores", "Rivera",
    "Silva", "Mendoza", "Castro", "Chavez", "Rojas", "Vargas", "Herrera",
    "Morales", "Cruz", "Reyes", "Jimenez", "Diaz", "Romero", "Gutierrez",
    "Ruiz", "Alvarez", "Castillo"
};

static const std::vector<std::string> calles = {
    "Av. Arequipa", "Av. Brasil", "Jr. Lampa", "Av. Petit Thouars",
    "Av. Javier Prado", "Av. La Marina", "Jr. Carabaya", "Av. Venezuela",
    "Av. Universitaria", "Av. Abancay", "Jr. Union", "Av. Colonial",
    "Av. Angamos", "Av. Salaverry", "Av. Tacna", "Av. Alfonso Ugarte"
};

static DNI_t generar_dni() {
    std::uniform_int_distribution<DNI_t> dist(10000000, 99999999);
    return dist(gen);
}

static std::string generar_nombre() {
    std::uniform_int_distribution<size_t> dist(0, nombres.size() - 1);
    return nombres[dist(gen)];
}

static std::string generar_apellido() {
    std::uniform_int_distribution<size_t> dist(0, apellidos.size() - 1);
    return apellidos[dist(gen)];
}

static std::string generar_direccion() {
    std::uniform_int_distribution<size_t> dist_calle(0, calles.size() - 1);
    std::uniform_int_distribution<int> dist_num(100, 9999);
    return calles[dist_calle(gen)] + " " + std::to_string(dist_num(gen));
}

static Ciudadano generar_ciudadano_aleatorio() {
    DNI_t dni = generar_dni();
    std::string nombre = generar_nombre();
    std::string apellido1 = generar_apellido();
    std::string apellido2 = generar_apellido();
    std::string apellidos = apellido1 + " " + apellido2;
    std::string direccion = generar_direccion();

    return Ciudadano(dni, nombre, apellidos, direccion);
}

void carga_masiva_test(Database& db, int cantidad) {
    if (cantidad <= 0) {
        std::cout << "\nError: La cantidad debe ser mayor a 0\n" << std::flush;
        return;
    }

    std::cout << "\nInsertando " << cantidad << " registros...\n" << std::flush;
    std::cout << "Progreso:\n" << std::flush;

    auto inicio = std::chrono::high_resolution_clock::now();

    int insertados = 0;
    int fallidos = 0;

    for (int i = 0; i < cantidad; i++) {
        Ciudadano c = generar_ciudadano_aleatorio();
        if (db.insertar_ciudadano(c)) {
            insertados++;
        } else {
            fallidos++;
            // Si falla (probablemente DNI duplicado), intentar con otro DNI
            for (int retry = 0; retry < 5; retry++) {
                c.dni = generar_dni();
                if (db.insertar_ciudadano(c)) {
                    insertados++;
                    fallidos--;
                    break;
                }
            }
        }

        // Mostrar resumen cada 100000 registros (o al final) para reducir overhead de IO
        if ((i + 1) % 100000 == 0 || (i + 1) == cantidad) {
            auto ahora = std::chrono::high_resolution_clock::now();
            auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - inicio);
            double registros_por_segundo = (double)(i + 1) / (duracion.count() > 0 ? duracion.count() : 1);

            std::cout << "\n>>> [" << (i + 1) << "/" << cantidad << "] "
                      << "Insertados: " << insertados
                      << " | Fallidos: " << fallidos
                      << " | Velocidad: " << (int)registros_por_segundo << " reg/s\n"
                      << std::flush;
        }
    }

    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);

    std::cout << "\n====== Resumen ======\n";
    std::cout << "Total registros insertados: " << insertados << "\n";
    std::cout << "Total fallidos: " << fallidos << "\n";
    std::cout << "Tiempo total: " << duracion.count() << " segundos\n";

    if (duracion.count() > 0) {
        std::cout << "Velocidad promedio: " << (insertados / duracion.count()) << " registros/segundo\n";
    }
}
