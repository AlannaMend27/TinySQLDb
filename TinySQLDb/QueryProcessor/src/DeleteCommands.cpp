#define _CRT_SECURE_NO_WARNINGS
#include "DeleteCommands.h"
#include <sstream>
#include <algorithm>

// Constructor
DeleteCommands::DeleteCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    : Commands(dataManager, catalog)
{
    //
}

// Ejecuta UPDATE <tabla> SET <columna> = <valor> [WHERE ...]
void DeleteCommands::executeDelete(QueryResult& result, const std::string& statement, const std::string& database)
{
    // extraer el nombre de la tabla
    std::string tableName = this->extractTableName(statement);

    // si el nombre esta vacio, la sintaxis fue incorrecta
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: UPDATE <tabla> SET <columna> = <valor>";
        return;
    }

    // validar que la base de datos y la tabla existan
    if (!this->validateDBTable(database, tableName, result))
    {
        return;
    }

    // obtener la tabla desde el system catalog
    Table table = this->systemCatalog.getTable(database, tableName);

    // parsear el WHERE si existe
    std::string whereColumn;
    std::string whereOperator;
    std::string whereValue;
    bool hasWhere = this->parseWhere(statement, whereColumn, whereOperator, whereValue);

    // si hay WHERE, verificar que la columna exista
    if (hasWhere && table.getColumn(whereColumn) == nullptr)
    {
        result.success = false;
        result.message = "Error: la columna '" + whereColumn + "' del WHERE no existe en la tabla";
        return;
    }

    // actualizar las filas que coinciden con las condiciones del statement, usando el data manager
    int deletedCount = this->deleteMatchingRows(table, whereColumn, whereOperator, whereValue, hasWhere);

    // actualizar mensaje de exito 
    result.success = true;
    result.message = std::to_string(deletedCount) + " fila(s) eliminadas(s)";
}

// extrae el nombre de la tabla donde se desea hacer el update
std::string DeleteCommands::extractTableName(const std::string& statement)
{
    std::istringstream stream(statement);
    std::string deleteWord;
    std::string from;
    std::string name;

    // saltamos DELETE y FROM y leemos el nombre de la tabla
    stream >> deleteWord >> from >> name;
    return name;
}

// recorre todas las filas, modifica las que cumplan el WHERE y las reescribe
int DeleteCommands::deleteMatchingRows(const Table& table, const std::string& whereColumn, 
                                       const std::string& whereOperator, const std::string& whereValue, 
                                       bool hasWhere)
{
    // pedir al dataManager todas las filas en un solo buffer
    int rowCount = 0;
    char* allRows = this->dataManager.readAllRows(table, rowCount);

    // si no hay filas, retornar 0
    if (allRows == nullptr)
    {
        return 0;
    }

    int deletedCount = 0;

    // recorrer cada fila dentro del buffer grande
    for (int i = 0; i < rowCount; i++)
    {
        // Nos desplzamos en el buffer allRows hasta donde empieza la fila de cada iteracion
        // Esta linea es importante pq le pasamos a los demas metodos el mismo buffer que tiene todas las filas
        // pero con el puntero apuntando al lugar donde incia la fila que se debe cambiar en caso de que haya match
        char* row = allRows + (i * table.rowSize);

        // saltar filas eliminadas
        if (row[0] == 0) {
            continue;
        }

        // verificar si la fila tiene where y si cumple el where
        if (!hasWhere || this->rowMatchesWhere(row, table, whereColumn, whereOperator, whereValue))
        {
            // marcar la fila como eliminada cambiando el flag a 0
            row[0] = 0;

            // pedir al dataManager que reescriba esta fila en su posicion
            this->dataManager.writeRowAt(table, i, row);

            deletedCount++;
        }
    }

    delete[] allRows;
    return deletedCount;
}
