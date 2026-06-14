#define _CRT_SECURE_NO_WARNINGS
#include "DeleteCommands.h"
#include <sstream>
#include <algorithm>

// Constructor
DeleteCommands::DeleteCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    : dataManager(dataManager), systemCatalog(catalog)
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

// valida que la base de datos y la tabla existan
bool DeleteCommands::validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result)
{
    // verificar que haya una base de datos activa
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return false;
    }

    // verificar que la base de datos exista
    if (!this->systemCatalog.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return false;
    }

    // verificar que la tabla exista
    if (!this->systemCatalog.tableExists(database, tableName))
    {
        result.success = false;
        result.message = "Error: la tabla '" + tableName + "' no existe";
        return false;
    }

    return true;
}

// extrae la condicion WHERE si existe
bool DeleteCommands::parseWhere(const std::string& statement, std::string& whereColumn, std::string& whereOperator, std::string& whereValue)
{
    // convertimos a mayusculas la statement
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // obtenemos el valor del where
    int wherePos = (int)upper.find(" WHERE ");

    // si no se encontro where
    if (wherePos == -1)
    {
        return false;
    }

    // obtener los caracteres luego del where
    std::string wherePart = statement.substr(wherePos + 7);

    // guardar los datos del where en las variables indicadas
    std::istringstream stream(wherePart);
    stream >> whereColumn >> whereOperator >> whereValue;

    // si el valor viene entre comillas, quitarlas
    if (!whereValue.empty() && whereValue.front() == '"')
    {
        whereValue.erase(whereValue.begin());
        if (!whereValue.empty() && whereValue.back() == '"')
            whereValue.pop_back();
    }

    return true;
}

// convierte los bytes de una columna a string legible
std::string DeleteCommands::deserializeValue(const char* buffer, const Column& col)
{
    // deseariza de acuerdo con el tipo de columna que se desea deserealizar
    switch (col.type)
    {
    case TYPE_INTEGER:
    {
        // convierte los datos int del buffer en un string
        int32_t num;
        memcpy(&num, buffer + col.offset, sizeof(int32_t));
        return std::to_string(num);

    }
    case TYPE_DOUBLE:
    {
        // convierte los datos de la columna de tipo doble del buffer en un string
        double num;
        memcpy(&num, buffer + col.offset, sizeof(double));
        return std::to_string(num);
    }
    case TYPE_VARCHAR:
    {
        // Creamos el string con el tamaño completo del buffer fijo
        std::string str(buffer + col.offset, col.size);

        // Buscamos dónde aparece el primer carácter nulo '\0'
        size_t nullPos = str.find('\0');

        // Si encontramos un nulo, recortamos el string hasta ahí para quitar el relleno basura
        if (nullPos != std::string::npos)
        {
            str = str.substr(0, nullPos);
        }

        // retornamos el string
        return str;
    }
    case TYPE_DATETIME:
    {
        // convierte el dato de tipo datetime d ela columna en un string legible
        int64_t timestamp;
        memcpy(&timestamp, buffer + col.offset, sizeof(int64_t));

        time_t t = (time_t)timestamp;
        struct tm* timeInfo = localtime(&t);

        // dar formato de string al tiempo con la funcion strftime
        char formatted[20];
        strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", timeInfo);

        // retornar string
        return std::string(formatted);
    }

    default:
        return "";

    }
}

// verifica si una fila cumple la condicion WHERE
bool DeleteCommands::rowMatchesWhere(const char* buffer, const Table& table,
    const std::string& whereColumn,
    const std::string& whereOperator,
    const std::string& whereValue)
{
    // obtener la columna de la statement
    const Column* col = table.getColumn(whereColumn);

    // si no se encontro la colmna, retornar falso
    if (col == nullptr)
    {
        return false;
    }

    // deserealizar el valor de la columna
    std::string cellValue = this->deserializeValue(buffer, *col);

    // pasar a mayusculas el operador toupper
    std::string op = whereOperator;
    std::transform(op.begin(), op.end(), op.begin(), ::toupper);

    if (op == "=")
    {
        // calmbiar el valor de esa celda
        return cellValue == whereValue;
    }
    else if (op == ">")
    {
        // en caso de que sean tipos de datos distintos, convertir antes de comparar
        if (col->type == TYPE_INTEGER || col->type == TYPE_DOUBLE)
        {
            return std::stod(cellValue) > std::stod(whereValue);
        }
        return cellValue > whereValue;
    }
    else if (op == "<")
    {
        // en caso de que sean tipos de datos distintos, convertir antes de comparar
        if (col->type == TYPE_INTEGER || col->type == TYPE_DOUBLE)
        {
            return std::stod(cellValue) < std::stod(whereValue);
        }
        return cellValue < whereValue;
    }
    else if (op == "LIKE")
    {
        // verificar si el valor de la celda es igual o contiene el patron dado
        std::string pattern = whereValue;
        while (!pattern.empty() && pattern.front() == '*')
        {
            pattern.erase(pattern.begin());
        }
        while (!pattern.empty() && pattern.back() == '*')
        {
            pattern.pop_back();
        }

        // Convertir el patrón limpio a mayúsculas
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::toupper);

        //Convertir también el valor de la celda a mayúsculas (en una variable temporal)
        std::string cellValueUpper = cellValue;
        std::transform(cellValueUpper.begin(), cellValueUpper.end(), cellValueUpper.begin(), ::toupper);

        // retornar si encontro el valor 
        return cellValueUpper.find(pattern) != std::string::npos;
    }
    else if (op == "NOT")
    {
        // retornar si son diferentes
        return cellValue != whereValue;
    }

    return false;
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
