#include "database.hpp"
#include "../test/generador_datos.hpp"
#include <iostream>
#include <string>
#include <limits>

void limpiar_buffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void mostrar_menu() {
    std::cout << "\n========== PeruDB - Sistema de Gestion de Ciudadanos ==========\n";
    std::cout << "1. Insertar ciudadano\n";
    std::cout << "2. Buscar ciudadano por DNI\n";
    std::cout << "3. Modificar ciudadano\n";
    std::cout << "4. Eliminar ciudadano\n";
    std::cout << "5. Carga masiva de datos\n";
    std::cout << "6. Salir\n";
    std::cout << "================================================================\n";
    std::cout << "Seleccione una opcion: ";
}

void insertar_ciudadano(Database& db) {
    DNI_t dni;
    std::string nombres, apellidos, direccion;

    std::cout << "\n--- Insertar Ciudadano ---\n";
    std::cout << "DNI: ";
    std::cin >> dni;
    limpiar_buffer();

    std::cout << "Nombres: ";
    std::getline(std::cin, nombres);

    std::cout << "Apellidos: ";
    std::getline(std::cin, apellidos);

    std::cout << "Direccion: ";
    std::getline(std::cin, direccion);

    Ciudadano c(dni, nombres, apellidos, direccion);

    if (db.insertar_ciudadano(c)) {
        std::cout << "\nCiudadano insertado correctamente.\n";
    } else {
        std::cout << "\nError: No se pudo insertar el ciudadano (puede que el DNI ya exista).\n";
    }
}

void buscar_ciudadano(Database& db) {
    DNI_t dni;

    std::cout << "\n--- Buscar Ciudadano ---\n";
    std::cout << "DNI: ";
    std::cin >> dni;
    limpiar_buffer();

    auto resultado = db.buscar_ciudadano(dni);

    if (resultado.has_value()) {
        const auto& c = resultado.value();
        std::cout << "\n--- Ciudadano Encontrado ---\n";
        std::cout << "DNI: " << c.dni << "\n";
        std::cout << "Nombres: " << c.nombres << "\n";
        std::cout << "Apellidos: " << c.apellidos << "\n";
        std::cout << "Direccion: " << c.direccion << "\n";
    } else {
        std::cout << "\nNo se encontro ningun ciudadano con DNI: " << dni << "\n";
    }
}

void modificar_ciudadano(Database& db) {
    DNI_t dni;

    std::cout << "\n--- Modificar Ciudadano ---\n";
    std::cout << "DNI del ciudadano a modificar: ";
    std::cin >> dni;
    limpiar_buffer();

    auto resultado = db.buscar_ciudadano(dni);

    if (!resultado.has_value()) {
        std::cout << "\nNo se encontro ningun ciudadano con DNI: " << dni << "\n";
        return;
    }

    std::cout << "\n--- Datos actuales ---\n";
    const auto& c_actual = resultado.value();
    std::cout << "Nombres: " << c_actual.nombres << "\n";
    std::cout << "Apellidos: " << c_actual.apellidos << "\n";
    std::cout << "Direccion: " << c_actual.direccion << "\n";

    std::string nombres, apellidos, direccion;

    std::cout << "\n--- Nuevos datos (Enter para mantener) ---\n";
    std::cout << "Nombres: ";
    std::getline(std::cin, nombres);
    if (nombres.empty()) nombres = c_actual.nombres;

    std::cout << "Apellidos: ";
    std::getline(std::cin, apellidos);
    if (apellidos.empty()) apellidos = c_actual.apellidos;

    std::cout << "Direccion: ";
    std::getline(std::cin, direccion);
    if (direccion.empty()) direccion = c_actual.direccion;

    Ciudadano c_nuevo(dni, nombres, apellidos, direccion);

    if (db.modificar_ciudadano(c_nuevo)) {
        std::cout << "\nCiudadano modificado correctamente.\n";
    } else {
        std::cout << "\nError: No se pudo modificar el ciudadano.\n";
    }
}

void eliminar_ciudadano(Database& db) {
    DNI_t dni;

    std::cout << "\n--- Eliminar Ciudadano ---\n";
    std::cout << "DNI del ciudadano a eliminar: ";
    std::cin >> dni;
    limpiar_buffer();

    auto resultado = db.buscar_ciudadano(dni);

    if (!resultado.has_value()) {
        std::cout << "\nNo se encontro ningun ciudadano con DNI: " << dni << "\n";
        return;
    }

    const auto& c = resultado.value();
    std::cout << "\n--- Ciudadano a eliminar ---\n";
    std::cout << "DNI: " << c.dni << "\n";
    std::cout << "Nombres: " << c.nombres << "\n";
    std::cout << "Apellidos: " << c.apellidos << "\n";

    std::cout << "\nEsta seguro? (s/n): ";
    char confirmacion;
    std::cin >> confirmacion;
    limpiar_buffer();

    if (confirmacion == 's' || confirmacion == 'S') {
        if (db.eliminar_ciudadano(dni)) {
            std::cout << "\nCiudadano eliminado correctamente.\n";
        } else {
            std::cout << "\nError: No se pudo eliminar el ciudadano.\n";
        }
    } else {
        std::cout << "\nOperacion cancelada.\n";
    }
}

void carga_masiva(Database& db) {
    int cantidad;

    std::cout << "\n--- Carga Masiva de Datos ---\n";
    std::cout << "Cantidad de registros a insertar: ";
    std::cin >> cantidad;
    limpiar_buffer();

    if (cantidad <= 0) {
        std::cout << "\nError: La cantidad debe ser mayor a 0\n";
        return;
    }

    std::cout << "\nEsta seguro? Se insertaran " << cantidad << " registros aleatorios (s/n): ";
    char confirmacion;
    std::cin >> confirmacion;
    limpiar_buffer();

    if (confirmacion != 's' && confirmacion != 'S') {
        std::cout << "\nOperacion cancelada.\n";
        return;
    }

    carga_masiva_test(db, cantidad);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo.db>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    Database db;

    try {
        if (!db.abrir(db_path)) {
            std::cerr << "Error: No se pudo abrir o crear el archivo de la base de datos: " << db_path << std::endl;
            return 1;
        }

        std::cout << "Base de datos abierta: " << db_path << "\n";

        int opcion;
        bool salir = false;

        while (!salir) {
            mostrar_menu();
            std::cin >> opcion;

            if (std::cin.fail()) {
                limpiar_buffer();
                std::cout << "\nOpcion invalida. Por favor ingrese un numero.\n";
                continue;
            }

            limpiar_buffer();

            switch (opcion) {
                case 1:
                    insertar_ciudadano(db);
                    break;
                case 2:
                    buscar_ciudadano(db);
                    break;
                case 3:
                    modificar_ciudadano(db);
                    break;
                case 4:
                    eliminar_ciudadano(db);
                    break;
                case 5:
                    carga_masiva(db);
                    break;
                case 6:
                    salir = true;
                    std::cout << "\nHasta luego!\n";
                    break;
                default:
                    std::cout << "\nOpcion invalida. Seleccione una opcion del 1 al 6.\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Ocurrio un error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
