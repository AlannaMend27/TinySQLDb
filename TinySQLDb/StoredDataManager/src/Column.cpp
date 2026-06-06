#include "column.h"
#include <regex>

// Constructores

// constructor que inicializa con valores predeterminados por defecto 
Column::Column()
    : type(TYPE_INTEGER), size(0), offset(0), position(0) {
}

Column::Column(const std::string& name, const std::string& tableName, ColumnType type, uint32_t size, uint32_t offset, uint32_t position)
{
    this->name = name;
    this->tableName = tableName;
    this->type = type;
    this->size = size;
    this->offset = offset;
    this->position = position;
}

// convierte un enum a un string legible para el cliente
std::string Column::typeToString() const {
    switch (this->type) {

    case TYPE_INTEGER:  
        return "INTEGER";

    case TYPE_DOUBLE:   
        return "DOUBLE";

    case TYPE_VARCHAR:  
        return "VARCHAR(" + std::to_string(size) + ")";

    case TYPE_DATETIME: 
        return "DATETIME";

    default:            
        return "UNKNOWN";
    }
}

// verificar si un string es compatible con el tipo de esa columna
bool Column::isValueCompatible(const std::string& value) const {
    switch (this->type) {

    case TYPE_INTEGER: {
        if (!this->StringToNum(value, TYPE_INTEGER)) {
            return false;
        }
        return true;
    }
    case TYPE_DOUBLE: {
        if (!this->StringToNum(value, TYPE_DOUBLE)) {
            return false;
        }
        return true;
    }

    case TYPE_VARCHAR: {

        // verificar que no exceda el tamanio establecido 
        return value.size() <= this->size;
    }

    case TYPE_DATETIME: {

        // validar el formato "YYYY-MM-DD HH:MM:SS"
        std::regex datePattern( R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");

        return std::regex_match(value, datePattern);
    }

    default:
        return false;
    }
}

// verifica que un string es un numero (ya sea interger o double)
bool Column::StringToNum(const std::string& value, ColumnType typeOfNum)const
{
    //verificar que la cadena no este vacia
    if (value.empty()) {
        return false;
    }

    char* endPtr = nullptr;

    if (typeOfNum == TYPE_INTEGER) {
        // Intentar la conversion a interger con base decimal
        std::strtol(value.c_str(), &endPtr, 10);
    }
    else {
        // intentar la conversion a double
        std::strtod(value.c_str(), &endPtr);
    }

    // Si endPtr se quedó al inicio, significa que no se pudo leer ningún número (ej: "abc")
    if (endPtr == value.c_str()) {
        return false;
    }

    // Si el caracter donde se detuvo NO es el nulo ('\0'), había basura al final (ej: "123abc")
    if (*endPtr != '\0') {
        return false;
    }
    return true;
}

// devuelve el tamanio fijo en bytes para un tipo de dato no variable
uint32_t Column::defaultSizeForType(ColumnType type) {

    switch (type) {
    case TYPE_INTEGER: 
        return 4;  // int32_t
    case TYPE_DOUBLE:   
        return 8;  // double estándar IEEE 754
    case TYPE_DATETIME: 
        return 8;  // int64_t Unix timestamp
    case TYPE_VARCHAR:  
        return 0;  // variable — viene del CREATE TABLE
    default:            
        return 0;
    }
}