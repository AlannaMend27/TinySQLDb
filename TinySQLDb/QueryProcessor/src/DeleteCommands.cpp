#define _CRT_SECURE_NO_WARNINGS
#include "DeleteCommands.h"
#include <sstream>
#include <algorithm>

// Constructor
DeleteCommands::DeleteCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager)
    : Commands(dataManager, catalog), indexManager(indexManager)
{
    //
}

// Ejecuta DELETE FROM <tabla> [WHERE ...]
void DeleteCommands::executeDelete(QueryResult& result, const std::string& statement, const std::string& database)
{
    // extraer el nombre de la tabla
    std::string tableName = this->extractTableName(statement);

    // si el nombre esta vacio, la sintaxis fue incorrecta
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: DELETE FROM <tabla>";
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

    // eliminar las filas que coinciden con las condiciones del statement
    int deletedCount = this->deleteMatchingRows(table, whereColumn, whereOperator, whereValue, hasWhere);

    // actualizar mensaje de exito 
    result.success = true;
    result.message = std::to_string(deletedCount) + " fila(s) eliminadas(s)";
}

// extrae el nombre de la tabla del statement
// DELETE FROM <nombre> → nombre
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

// elimina una fila usando el indice para ir directo a su posicion en disco
int DeleteCommands::deleteWithIndex(const Table& table, const std::string& whereColumn, const std::string& whereValue, char* allRows)
{
    // obtener el indice activo para la columna del WHERE
    ActiveIndex* activeIndex = this->indexManager.getIndex(table.name, whereColumn);
    long position = -1;

    if (activeIndex->type == INDEX_BST && activeIndex->BST != nullptr)
    {
        // buscar la posicion en el arbol BST
        position = activeIndex->BST->search(whereValue);
    }
    else if (activeIndex->bTree != nullptr)
    {
        // buscar la posicion en el arbol BTREE
        position = activeIndex->bTree->search(whereValue);
    }

    // si no se encontro el valor en el indice, no hay nada que eliminar
    if (position == -1)
    {
        return 0;
    }

    // calcular el indice de la fila a partir de la posicion en disco
    int rowIndex = (int)(position / table.rowSize);

    // Nos desplazamos en el buffer allRows hasta donde empieza la fila
    char* row = allRows + (rowIndex * table.rowSize);

    // marcar la fila como eliminada cambiando el flag a 0
    row[0] = 0;

    // pedir al dataManager que reescriba esta fila en su posicion
    this->dataManager.writeRowAt(table, rowIndex, row);

    return 1;
}

// elimina filas usando busqueda secuencial
int DeleteCommands::deleteSequential(const Table& table, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue, bool hasWhere, char* allRows, int rowCount)
{
    int deletedCount = 0;

    // recorrer cada fila dentro del buffer grande
    for (int i = 0; i < rowCount; i++)
    {
        // Nos desplazamos en el buffer allRows hasta donde empieza la fila de cada iteracion
        char* row = allRows + (i * table.rowSize);

        // saltar filas eliminadas
        if (row[0] == 0)
        {
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

    return deletedCount;
}

// recorre las filas y elimina las que cumplan el WHERE
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

    // verificar si hay indice en la columna del WHERE para busqueda directa
    bool useIndex = hasWhere && whereOperator == "=" && this->indexManager.hasIndex(table.name, whereColumn);

    if (useIndex)
    {
        // usar el indice para ir directo a la fila
        deletedCount = this->deleteWithIndex(table, whereColumn, whereValue, allRows);
    }
    else
    {
        // busqueda secuencial
        deletedCount = this->deleteSequential(table, whereColumn, whereOperator, whereValue, hasWhere, allRows, rowCount);
    }

    delete[] allRows;

    // reconstruir todos los indices de la tabla si se eliminaron filas
    if (deletedCount > 0)
    {
        for (int i = 0; i < (int)table.columnCount; i++)
        {
            if (this->indexManager.hasIndex(table.name, table.columns[i].name))
            {
                // reconstruir el indice para mantenerlo actualizado
                this->indexManager.rebuildIndex(table.name, table.columns[i].name, this->systemCatalog, this->dataManager);
            }
        }
    }

    return deletedCount;
}