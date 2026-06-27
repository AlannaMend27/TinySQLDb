#include "Database.h"
#include <regex>

// constructores

Database::Database(){
    this->name = "";
}

Database::Database(const std::string& name)
{
    this->name = name;
}

// determina si el nombre de la base de datos es valido
bool Database::isValid() {

    // verificar que el nombre de la base de datos sea valido
    if (name.empty()) {
        return false;
    }

    // verificar que uno o mas caracteres sean letras, numeros o guion bajo
    std::regex patron("^[a-zA-Z0-9_]+$");

    // retorna si el nombre es valido
    return std::regex_match(name, patron);
    
}

//retorna el nombre de la base de datos
std::string Database::toString(){
    return "Database(" + name + ")";
}

