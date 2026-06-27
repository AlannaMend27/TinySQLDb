#define _CRT_SECURE_NO_WARNINGS
#include "IndexCommands.h"
#include <sstream>
#include <algorithm>

// Constructor
IndexCommands::IndexCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager)
    : Commands(dataManager, catalog), indexManager(indexManager)
{
}

// Ejecuta CREATE INDEX <nombre> ON <tabla>(<columna>) OF TYPE <tipo>
void IndexCommands::executeCreateIndex(QueryResult& result, const std::string& statement, const std::string& database)
{
    // extraer los componentes del statement
    std::string indexName = this->extractIndexName(statement);
    std::string tableName = this->extractTableName(statement);
    std::string columnName = this->extractColumnName(statement);
    IndexType   indexType = this->extractIndexType(statement);

    // verificar que se hayan extraido correctamente
    if (indexName.empty() || tableName.empty() || columnName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: CREATE INDEX <nombre> ON <tabla>(<columna>) OF TYPE BST|BTREE";
        return;
    }

    // validar que la base de datos y la tabla existan
    if (!this->validateDBTable(database, tableName, result))
    {
        return;
    }

    // obtener la tabla desde el system catalog
    Table table = this->systemCatalog.getTable(database, tableName);

    // verificar que la columna exista en la tabla
    const Column* col = table.getColumn(columnName);
    if (col == nullptr)
    {
        result.success = false;
        result.message = "Error: la columna '" + columnName + "' no existe en la tabla '" + tableName + "'";
        return;
    }

    // verificar que no haya ya un indice en esa columna
    if (this->indexManager.hasIndex(tableName, columnName))
    {
        result.success = false;
        result.message = "Error: ya existe un indice en la columna '" + columnName + "'";
        return;
    }

    // si la tabla tiene datos, verificar que no haya duplicados
    if (!this->columnHasNoDuplicates(table, *col))
    {
        result.success = false;
        result.message = "Error: la columna '" + columnName + "' tiene valores duplicados. No se puede crear el indice";
        return;
    }

    // crear el arbol en memoria segun el tipo de indice
    BST* bstTree = nullptr;
    BTree* btreeTree = nullptr;

    if (indexType == INDEX_BST)
    {
        bstTree = new BST(col->type);
    }
    else
    {
        btreeTree = new BTree(col->type);
    }

    // leer todas las filas de la tabla y llenar el arbol
    int rowCount = 0;
    char* allRows = this->dataManager.readAllRows(table, rowCount);

    if (allRows != nullptr)
    {
        for (int i = 0; i < rowCount; i++)
        {
            // nos movemos de fila en fila
            char* rowPtr = allRows + (i * table.rowSize);

            // saltar filas eliminadas
            if (rowPtr[0] == 0)
            {
                continue;
            }

            // deserializar el valor de la columna indexada
            std::string key = Commands::deserializeValue(rowPtr, *col);

            // calcular la posicion en disco de esta fila
            long position = (long)i * table.rowSize;

            //insertar en el arbol
            if (indexType == INDEX_BST)
            {
                bstTree->insert(key, position);
            }
            else
            {
                btreeTree->insert(key, position);
            }
        }

        delete[] allRows;
    }

    // registrar el indice en el system catalog
    Index index(indexName, tableName, columnName, indexType);
    this->systemCatalog.registerIndex(index);

    // agregar el indice al manager en memoria
    this->indexManager.addIndex(indexName, tableName, columnName, indexType, bstTree, btreeTree);

    result.success = true;
    result.message = "Indice '" + indexName + "' creado exitosamente en '" + tableName + "(" + columnName + ")'";
}

// Extrae el nombre del indice del statement
std::string IndexCommands::extractIndexName(const std::string& statement)
{
    std::istringstream stream(statement);
    std::string create;
    std::string index;
    std::string name;

    // saltamos CREATE y INDEX y leemos el nombre
    stream >> create >> index >> name;
    return name;
}

// Extrae el nombre de la tabla del statement
std::string IndexCommands::extractTableName(const std::string& statement)
{
    // convertir a mayusculas para buscar ON
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion de ON
    int onPos = (int)upper.find(" ON ");

    //si no se encontro
    if (onPos == -1)
    {
        return "";
    }

    // extraer la parte despues de ON
    std::string afterOn = statement.substr(onPos + 4);

    // leer hasta el parentesis
    std::istringstream stream(afterOn);
    std::string tablePart;
    stream >> tablePart;

    // quitar el parentesis y lo que sigue
    int parenPos = (int)tablePart.find('(');
    if (parenPos != -1)
    {
        tablePart = tablePart.substr(0, parenPos);
    }

    return tablePart;
}

// Extrae el nombre de la columna entre parentesis
std::string IndexCommands::extractColumnName(const std::string& statement)
{
    // buscar el parentesis que abre
    int open = (int)statement.find('(');
    int close = (int)statement.find(')');

    if (open == -1 || close == -1 || close <= open)
    {
        return "";
    }

    // extraer lo que esta entre los parentesis
    return statement.substr(open + 1, close - open - 1);
}

// Extrae el tipo de indice del statement btree o bst
IndexType IndexCommands::extractIndexType(const std::string& statement)
{
    // convertir a mayusculas para comparar
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar OF TYPE
    int typePos = (int)upper.find(" OF TYPE ");
    if (typePos == -1)
    {
        return INDEX_BST;
    }

    // extraer la palabra despues de OF TYPE
    std::string typePart = upper.substr(typePos + 9);
    std::istringstream stream(typePart);
    std::string typeName;
    stream >> typeName;

    if (typeName == "BTREE")
    {
        return INDEX_BTREE;
    }

    return INDEX_BST;
}

// Verifica que no haya valores duplicados en la columna
bool IndexCommands::columnHasNoDuplicates(const Table& table, const Column& col)
{
    int rowCount = 0;
    char* allRows = this->dataManager.readAllRows(table, rowCount);

    if (allRows == nullptr)
    {
        return true;
    }

    // usar un BST temporal para detectar duplicados
    BST tempTree(col.type);

    for (int i = 0; i < rowCount; i++)
    {
        char* rowPtr = allRows + (i * table.rowSize);

        // saltar filas eliminadas
        if (rowPtr[0] == 0)
        {
            continue;
        }

        // deserializar el valor
        std::string key = Commands::deserializeValue(rowPtr, col);

        // si ya existe en el arbol temporal, hay duplicado
        if (tempTree.valueExists(key))
        {
            delete[] allRows;
            return false;
        }

        tempTree.insert(key, i);
    }

    delete[] allRows;
    return true;
}