#define _CRT_SECURE_NO_WARNINGS
#include "UpdateCommands.h"
#include <sstream>
#include <algorithm>

// Constructor
UpdateCommands::UpdateCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager)
    : Commands(dataManager, catalog), indexManager(indexManager)
{
    //
}

// Ejecuta UPDATE <tabla> SET <columna> = <valor> [WHERE ...]
void UpdateCommands::executeUpdate(QueryResult& result, const std::string& statement, const std::string& database)
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

    // parsear el SET
    std::string setColumn;
    std::string setValue;
    if (!this->parseSet(statement, setColumn, setValue, result))
    {
        return;
    }

    // verificar que la columna del SET exista en la tabla
    const Column* col = table.getColumn(setColumn);

    // si la columna no existe, error de sintaxis
    if (col == nullptr)
    {
        result.success = false;
        result.message = "Error: la columna '" + setColumn + "' no existe en la tabla '" + setValue + "'.";
        return;
    }

    // verificar que el nuevo valor sea compatible con el tipo de la columna
    if (!col->isValueCompatible(setValue))
    {
        result.success = false;
        result.message = "Error: el valor '" + setValue + "' no es compatible con la columna '" + setColumn + "' de tipo " + col->typeToString();
        return;
    }

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
    int updatedCount = this->updateMatchingRows(table, *col, setValue, whereColumn, whereOperator, whereValue, hasWhere);

    // actualizar mensaje de exito 
    result.success = true;
    result.message = std::to_string(updatedCount) + " fila(s) actualizada(s)";
}

// extrae el nombre de la tabla donde se desea hacer el update
std::string UpdateCommands::extractTableName(const std::string& statement)
{
    std::istringstream stream(statement);
    std::string update;
    std::string name;

    // saltamos UPDATE y leemos el nombre de la tabla
    stream >> update >> name;
    return name;
}

// extrae la columna y el valor del SET
// SET Nombre = "Felipe"  setColumn="Nombre", setValue="Felipe"
bool UpdateCommands::parseSet(const std::string& statement, std::string& setColumn, std::string& setValue, QueryResult& result)
{
    // convertir a mayusculas para buscar SET
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion de SET en la statement colocada por el usuario
    int setPos = (int)upper.find(" SET ");

    // si no se encontro SET en al statement
    if (setPos == -1)
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: UPDATE <tabla> SET <columna> = <valor>";
        return false;
    }

    // extraer la parte despues de SET
    // puede terminar en WHERE o en el final del string
    int wherePos = (int)upper.find(" WHERE ");
    std::string setPart;

    // si se encontro el where y esta despues de la pos del set
    if (wherePos != -1 && wherePos > setPos)
        // obtener lo que esta entre el set y entre el where 
        setPart = statement.substr(setPos + 5, wherePos - setPos - 5);
    else
        // obtener lo que esta luego del set
        setPart = statement.substr(setPos + 5);

    // buscar el signo =
    int equalPos = (int)setPart.find('=');
    if (equalPos == -1)
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Falta el '=' en el SET";
        return false;
    }

    // extraer columna (antes del =)
    setColumn = setPart.substr(0, equalPos);
    while (!setColumn.empty() && setColumn.front() == ' ') setColumn.erase(setColumn.begin());
    while (!setColumn.empty() && setColumn.back() == ' ') setColumn.pop_back();

    // extraer valor (despues del =)
    setValue = setPart.substr(equalPos + 1);
    while (!setValue.empty() && setValue.front() == ' ') setValue.erase(setValue.begin());
    while (!setValue.empty() && setValue.back() == ' ') setValue.pop_back();

    // quitar comillas simples o dobles
    setValue = stripQuotes(setValue);

    if (setColumn.empty() || setValue.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta en el SET";
        return false;
    }

    return true;
}

// actualiza filas usando el indice para busqueda directa
int UpdateCommands::updateWithIndex(const Table& table, const Column& setCol, const std::string& setValue, const std::string& whereColumn, const std::string& whereValue, char* allRows)
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

    // si no se encontro el valor en el indice, no hay nada que actualizar
    if (position == -1)
    {
        return 0;
    }

    // calcular el indice de la fila a partir de la posicion en disco
    int rowIndex = (int)(position / table.rowSize);
    char* row = allRows + (rowIndex * table.rowSize);

    // modificar el valor en el buffer
    this->serializeSingleValue(row, setCol, setValue);

    // reescribir la fila en disco
    this->dataManager.writeRowAt(table, rowIndex, row);

    return 1;
}

// actualiza filas usando busqueda secuencial
int UpdateCommands::updateSequential(const Table& table, const Column& setCol, const std::string& setValue,
    const std::string& whereColumn, const std::string& whereOperator,const std::string& whereValue, bool hasWhere, char* allRows, int rowCount)
{
    int updatedCount = 0;

    // recorrer cada fila dentro del buffer grande
    for (int i = 0; i < rowCount; i++)
    {

        // Nos desplzamos en el buffer allRows hasta donde empieza la fila de cada iteracion
        // Esta linea es importante pq le pasamos a los demas metodos el mismo buffer que tiene todas las filas
        // pero con el puntero apuntando al lugar donde incia la fila que se debe cambiar en caso de que haya match
        char* row = allRows + (i * table.rowSize);

        // saltar filas eliminadas
        if (row[0] == 0)
        {
            continue;
        }

        // verificar si la fila tiene where y si cumple el where
        if (!hasWhere || this->rowMatchesWhere(row, table, whereColumn, whereOperator, whereValue))
        {
            // modificar el valor en el buffer de esta fila 
            this->serializeSingleValue(row, setCol, setValue);

            // pedir al dataManager que reescriba esta fila en su posicion
            this->dataManager.writeRowAt(table, i, row);
            updatedCount++;
        }
    }

    return updatedCount;
}

// recorre las filas y actualiza las que cumplan el WHERE
int UpdateCommands::updateMatchingRows(const Table& table, const Column& setCol, const std::string& setValue,
    const std::string& whereColumn, const std::string& whereOperator,
    const std::string& whereValue, bool hasWhere)
{
    // leer todas las filas en un buffer
    int rowCount = 0;
    char* allRows = this->dataManager.readAllRows(table, rowCount);

    if (allRows == nullptr)
    {
        return 0;
    }

    int updatedCount = 0;

    // verificar si hay indice en la columna del WHERE para busqueda directa
    bool useIndex = hasWhere && whereOperator == "=" && this->indexManager.hasIndex(table.name, whereColumn);

    if (useIndex)
    {
        // usar el indice para ir directo a la fila
        updatedCount = this->updateWithIndex(table, setCol, setValue, whereColumn, whereValue, allRows);
    }
    else
    {
        // busqueda secuencial
        updatedCount = this->updateSequential(table, setCol, setValue, whereColumn, whereOperator, whereValue, hasWhere, allRows, rowCount);
    }

    delete[] allRows;

    // si la columna actualizada tiene un indice, reconstruirlo
    if (updatedCount > 0 && this->indexManager.hasIndex(table.name, setCol.name))
    {
        this->indexManager.rebuildIndex(table.name, setCol.name, this->systemCatalog, this->dataManager);
    }

    return updatedCount;
}