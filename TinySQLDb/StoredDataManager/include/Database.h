#pragma once
#include <string>

// Database.h -> representa una base de datos en memoria

class Database {
public:
    std::string name; 

    // Constructor vacío
    Database();

    // Constructor (explicito para evitar conversiones al ser un solo parametro)
    explicit Database(const std::string& name);
    bool isValid();
    std::string toString();
};
