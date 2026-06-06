#pragma once
#include <string>
#include "Records.h"

// column.h -> representa una columna de una tabla en memoria

class Column {
public:
    // Atributos publicos 
    std::string name;
    std::string tableName;
    ColumnType  type;
    uint32_t    size;       // cuántos bytes ocupa en la fila
    uint32_t    offset;     // posición en bytes dentro de la fila
    uint32_t    position;   // orden de la columna: 0, 1, 2...

    // constructor y destructor
    Column();
    Column(const std::string& name, const std::string& tableName,ColumnType type, uint32_t size, uint32_t offset, uint32_t position);

    // metodos publicos 
    std::string typeToString() const;
    bool isValueCompatible(const std::string& value) const;
    bool StringToNum(const std::string& value, ColumnType typeOfNum)const;
    static uint32_t defaultSizeForType(ColumnType type);
};