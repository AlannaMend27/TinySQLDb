#pragma once
#include <string>
#include "Column.h"

const uint32_t MAX_COLUMNS = 50;
const uint32_t MAX_ROWS = 200;

class Table {
public:

    // Atributos publicos
    std::string name;     
    std::string dbName;    
    uint32_t rowSize; // tamaño fijo de cada fila en bytes
    Column columns[MAX_COLUMNS]; // columnas en orden de posición
    uint32_t columnCount;

    // Constructor vacío
    Table();

    // Constructor completo
    Table(const std::string& name, const std::string& dbName, Column* columnsArray, uint32_t count);

    // metodos publicos
    void calculateRowSize();
    const Column* getColumn(const std::string& colName) const;
    bool hasColumn(const std::string& colName) const;
    bool isValid() const;

};
