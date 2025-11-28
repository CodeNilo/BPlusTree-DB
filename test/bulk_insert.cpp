#include "database.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <string>

// Generador de numeros aleatorios
std::random_device rd;
std::mt19937 gen(rd());

// Listas de nombres y apellidos peruanos comunes
const std::vector<std::string> nombres = {
    "Juan", "Maria", "Carlos", "Ana", "Luis", "Rosa", "Jorge", "Carmen",
    "Pedro", "Lucia", "Miguel", "Sofia", "Jose", "Isabel", "Ricardo",
    "Elena", "Fernando", "Patricia", "Roberto", "Teresa", "Alberto", "Laura",
    "Manuel", "Gloria", "Antonio", "Marta", "Francisco", "Diana", "Diego", "Sandra"
};

const std::vector<std::string> apellidos = {
    "Garcia", "Rodriguez", "Martinez", "Fernandez", "Lopez", "Gonzalez",
    "Sanchez", "Perez", "Gomez", "Torres", "Ramirez", "Flores", "Rivera",
    "Silva", "Mendoza", "Castro", "Chavez", "Rojas", "Vargas", "Herrera",
    "Morales", "Cruz", "Reyes", "Jimenez", "Diaz", "Romero", "Gutierrez",
    "Ruiz", "Alvarez", "Castillo"
};

const std::vector<std::string> calles = {
    "Av. Arequipa", "Av. Brasil", "Jr. Lampa", "Av. Petit Thouars",
    "Av. Javier Prado", "Av. La Marina", "Jr. Carabaya", "Av. Venezuela",
    "Av. Universitaria", "Av. Abancay", "Jr. Union", "Av. Colonial",
    "Av. Angamos", "Av. Salaverry", "Av. Tacna", "Av. Alfonso Ugarte"
};

// Genera un DNI aleatorio entre 10000000 y 99999999
DNI_t generar_dni() {
    std::uniform_int_distribution<DNI_t> dist(10000000, 99999999);
    return dist(gen);
}

// Genera un nombre aleatorio
std::string generar_nombre() {
    std::uniform_int_distribution<size_t> dist(0, nombres.size() - 1);
    return nombres[dist(gen)];
}

// Genera un apellido aleatorio
std::string generar_apellido() {
    std::uniform_int_distribution<size_t> dist(0, apellidos.size() - 1);
    return apellidos[dist(gen)];
}

// Genera una direccion aleatoria
std::string generar_direccion() {
    std::uniform_int_distribution<size_t> dist_calle(0, calles.size() - 1);
    std::uniform_int_distribution<int> dist_num(100, 9999);

    return calles[dist_calle(gen)] + " " + std::to_string(dist_num(gen));
}

// Genera un ciudadano aleatorio
Ciudadano generar_ciudadano_aleatorio(DNI_t dni) {
    std::string nombre = generar_nombre();
    std::string apellido1 = generar_apellido();
    std::string apellido2 = generar_apellido();
    std::string apellidos = apellido1 + " " + apellido2;
    std::string direccion = generar_direccion();

    return Ciudadano(dni, nombre, apellidos, direccion);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <archivo.db> <cantidad_registros>" << std::endl;
        std::cerr << "Ejemplo: " << argv[0] << " test.db 1000000" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    int cantidad = std::atoi(argv[2]);

    if (cantidad <= 0) {
        std::cerr << "Error: La cantidad debe ser mayor a 0" << std::endl;
        return 1;
    }

    std::cout << "Inicializando base de datos: " << db_path << std::endl;

    Database db;

    try {
        if (!db.abrir(db_path)) {
            std::cerr << "Error: No se pudo abrir o crear la base de datos" << std::endl;
            return 1;
        }

        std::cout << "Base de datos abierta correctamente" << std::endl;
        std::cout << "Insertando " << cantidad << " registros..." << std::endl;
        std::cout << "Progreso:" << std::endl;

        auto inicio = std::chrono::high_resolution_clock::now();

        int insertados = 0;
        int fallidos = 0;

        for (int i = 0; i < cantidad; i++) {
            DNI_t dni = generar_dni();
            Ciudadano c = generar_ciudadano_aleatorio(dni);

            if (db.insertar_ciudadano(c)) {
                insertados++;
            } else {
                fallidos++;
                // Si falla (probablemente DNI duplicado), intentar con otro DNI
                for (int retry = 0; retry < 5; retry++) {
                    dni = generar_dni();
                    c.dni = dni;
                    if (db.insertar_ciudadano(c)) {
                        insertados++;
                        fallidos--;
                        break;
                    }
                }
            }

            // Mostrar progreso cada 10000 registros
            if ((i + 1) % 10000 == 0) {
                auto ahora = std::chrono::high_resolution_clock::now();
                auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - inicio);
                double registros_por_segundo = (double)(i + 1) / (duracion.count() > 0 ? duracion.count() : 1);

                std::cout << "  [" << (i + 1) << "/" << cantidad << "] "
                          << "Insertados: " << insertados
                          << " | Fallidos: " << fallidos
                          << " | Velocidad: " << (int)registros_por_segundo << " reg/s"
                          << std::endl;
            }
        }

        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);

        std::cout << "\n====== Resumen ======" << std::endl;
        std::cout << "Total registros insertados: " << insertados << std::endl;
        std::cout << "Total fallidos: " << fallidos << std::endl;
        std::cout << "Tiempo total: " << duracion.count() << " segundos" << std::endl;

        if (duracion.count() > 0) {
            std::cout << "Velocidad promedio: " << (insertados / duracion.count()) << " registros/segundo" << std::endl;
        }

        std::cout << "\nBase de datos cerrada correctamente." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
