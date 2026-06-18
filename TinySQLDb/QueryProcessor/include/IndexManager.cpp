#define _CRT_SECURE_NO_WARNINGS
#include "IndexManager.h"
#include <fstream>
#include <algorithm>


// Constructor
IndexManager::IndexManager()
{
    this->indexCount = 0;
}

// Destructor, birra los arboles en memoria
IndexManager::~IndexManager()
{
    for (int i = 0; i < this->indexCount; i++)
    {
        //BST
        if (this->indexes[i].BST != nullptr)
        {
            delete this->indexes[i].BST;
            this->indexes[i].BST = nullptr;
        }
        //BTree
        if (this->indexes[i].bTree != nullptr)
        {
            delete this->indexes[i].bTree;
            this->indexes[i].bTree = nullptr;
        }
    }
}

// Reconstruye todos los indices al iniciar el servidor, lee desde el system catalog
void IndexManager::loadFromCatalog(SystemCatalog& catalog, StoredDataManager& dataManager)
{
    // obtener todos los indices registrados en el system catalog
    int indexCount = 0;
    Index* allIndexes = catalog.getAllIndexes(indexCount);

    // si no hay indices registrados
    if (allIndexes == nullptr)
    {
        return;
    }

    // reconstruir cada indice en memoria
    for (int i = 0; i < indexCount; i++)
    {
        Index& index = allIndexes[i];

        // buscamos en SystemTables cual es la db de esa tabla
        std::string dbName = catalog.getDatabaseForTable(index.tableName);
        if (dbName.empty())
        {
            continue;
        }

        Table table = catalog.getTable(dbName, index.tableName);
        if (!table.isValid())
        {
            continue;
        }

        // obtener la columna indexada
        const Column* col = table.getColumn(index.columnName);
        if (col == nullptr)
        {
            continue;
        }

        // crear el arbol en memoria segun el tipo de indice
        BST* bstTree = nullptr;
        BTree* bTree = nullptr;

        if (index.type == INDEX_BST)
        {
            bstTree = new BST(col->type);
        }
        else
        {
            bTree = new BTree(col->type);
        }

        // leer todas las filas de la tabla y llenar el arbol
        int rowCount = 0;
        char* allRows = dataManager.readAllRows(table, rowCount);

        if (allRows != nullptr)
        {
            for (int row = 0; row < rowCount; row++)
            {
                //calcular donde empiezxa la fila dentro del buffer completo
                char* rowPtr = allRows + (row * table.rowSize);

                // saltar filas eliminadas
                if (rowPtr[0] == 0)
                {
                    continue;
                }

                // deserializar el valor de la columna indexada
                std::string key = this->deserializeForIndex(rowPtr, *col);

                // calcular la posicion en disco de esta fila
                long position = (long)row * table.rowSize;

                // insertar en el arbol correspondiente
                if (index.type == INDEX_BST)
                {
                    bstTree->insert(key, position);
                }
                else
                {
                    bTree->insert(key, position);
                }
            }

            delete[] allRows;
        }

        // agregar el indice reconstruido al manager
        this->addIndex(index.name, index.tableName, index.columnName, index.type, bstTree, bTree);
    }

    delete[] allIndexes;
}

// Agrega un indice nuevo en memoria
bool IndexManager::addIndex(const std::string& indexName, const std::string& tableName,const std::string& columnName, IndexType type, BST* bstTree, BTree* bTree)
{
    // verificar que no se supere el maximo de indices
    if (this->indexCount >= MAX_INDEXES)
    {
        return false;
    }

    // llenar el struct del indice activo
    ActiveIndex& active = this->indexes[this->indexCount];
    active.indexName = indexName;
    active.tableName = tableName;
    active.columnName = columnName;
    active.type = type;
    active.BST = bstTree;
    active.bTree = bTree;


    this->indexCount++;
    return true;
}

// Retorna el indice activo para una tabla y columna especifica
ActiveIndex* IndexManager::getIndex(const std::string& tableName, const std::string& columnName)
{
    for (int i = 0; i < this->indexCount; i++)
    {
        if (this->indexes[i].tableName == tableName &&
            this->indexes[i].columnName == columnName)
        {
            return &this->indexes[i];
        }
    }

    return nullptr;
}

// Verifica si existe un indice para una tabla y columna especifica
bool IndexManager::hasIndex(const std::string& tableName, const std::string& columnName)
{
    return this->getIndex(tableName, columnName) != nullptr;
}

// convierte los bytes de una columna a string legible para index
std::string IndexManager::deserializeForIndex(const char* buffer, const Column& col)
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
        struct tm timeInfoStruct;
        localtime_s(&timeInfoStruct, &t);

        // dar formato de string al tiempo con la funcion strftime
        char formatted[20];
        strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", &timeInfoStruct);

        // retornar string
        return std::string(formatted);
    }

    default:
        return "";

    }
}


// Reconstruye el arbol de un indice especifico desde los datos en disco
void IndexManager::rebuildIndex(const std::string& tableName, const std::string& columnName, SystemCatalog& catalog, StoredDataManager& dataManager)
{
    // obtener el indice activo
    ActiveIndex* activeIndex = this->getIndex(tableName, columnName);
    if (activeIndex == nullptr)
    {
        return;
    }

    // obtener la base de datos y la tabla
    std::string dbName = catalog.getDatabaseForTable(tableName);
    if (dbName.empty())
    {
        return;
    }

    Table table = catalog.getTable(dbName, tableName);
    if (!table.isValid())
    {
        return;
    }

    // obtener la columna indexada
    const Column* col = table.getColumn(columnName);
    if (col == nullptr)
    {
        return;
    }

    // limpiar el arbol actual y reconstruirlo
    if (activeIndex->type == INDEX_BST && activeIndex->BST != nullptr)
    {
        // para BST limpiar con clear
        activeIndex->BST->clear();
    }
    else if (activeIndex->bTree != nullptr)
    {
        // para BTree limpiar con clear
        activeIndex->bTree->clear();
    }

    // leer todas las filas activas y reinsertar en el arbol
    int rowCount = 0;
    char* allRows = dataManager.readAllRows(table, rowCount);

    if (allRows == nullptr)
    {
        return;
    }

    for (int i = 0; i < rowCount; i++)
    {
        char* rowPtr = allRows + (i * table.rowSize);

        // saltar filas eliminadas
        if (rowPtr[0] == 0)
        {
            continue;
        }

        // deserializar el valor de la columna indexada
        std::string key = this->deserializeForIndex(rowPtr, *col);

        // calcular la posicion en disco
        long position = (long)i * table.rowSize;

        // reinsertar en el arbol correspondiente
        if (activeIndex->type == INDEX_BST && activeIndex->BST != nullptr)
        {
            activeIndex->BST->insert(key, position);
        }
        else if (activeIndex->bTree != nullptr)
        {
            activeIndex->bTree->insert(key, position);
        }
    }

    delete[] allRows;
}