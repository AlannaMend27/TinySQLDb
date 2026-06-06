#include "Table.h"
#include <algorithm>


Table::Table()
{
    this->columnCount = 0;
    this->rowSize = 0;
}

Table::Table(const std::string& name, const std::string& dbName, Column* columnsArray, uint32_t count)  
{
    this->name = name;
    this->dbName = dbName;
    this->columnCount = count;
    for (uint32_t i = 0; i < count; i++) {
        this->columns[i] = columnsArray[i]; 
    }

    // calcular el tamanio que tendra cada fila en la tabla
    calculateRowSize();
}

// calcula el tamanio que tendra cada fila en la tabla 
void Table::calculateRowSize() {
    // iniciamos en tamanio en uno, ya que toda fila tendra un byte de flag (indica si la fila esta activa o no)
    rowSize = 1;

    for (uint32_t i = 0; i < this->columnCount; i++) {
        this->rowSize += this->columns[i].size;
    }
}

// devuelve la columna deseada de acuerdo con su nombre 
const Column* Table::getColumn(const std::string& colName) const {

    // recorremos todas las columnas para buscar el nombre
    for (uint32_t i = 0; i < this->columnCount; i++) {
        if (this->columns[i].name == colName) {
            return &(this->columns[i]);
        }
    }
    return nullptr;
}


// verifica si la columna existe en la tabla 
bool Table::hasColumn(const std::string& colName) const {
    // Reutiliza getColumn — si devuelve nullptr, no existe
    return getColumn(colName) != nullptr;
}

// verifica que la tabla tenga nombre, pertenezca a una base de datos y que tenga columnas
bool Table::isValid() const {
    return !name.empty() && !dbName.empty() && columns != nullptr && columnCount > 0;
}

