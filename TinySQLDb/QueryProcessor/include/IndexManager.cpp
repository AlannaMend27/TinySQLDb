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
        if (this->indexes[i].tree != nullptr)
        {
            delete this->indexes[i].tree;
            this->indexes[i].tree = nullptr;
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

        // crear el arbol en memoria
        BST* tree = new BST(col->type);

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
                std::string key = Commands::deserializeValue(rowPtr, *col);

                // calcular la posicion en disco de esta fila
                long position = (long)row * table.rowSize;

                // insertar en el arbol
                tree->insert(key, position);
            }

            delete[] allRows;
        }

        // agregar el indice reconstruido al manager
        this->addIndex(index.name, index.tableName, index.columnName, index.type, tree);
    }

    delete[] allIndexes;
}

// Agrega un indice nuevo en memoria
bool IndexManager::addIndex(const std::string& indexName, const std::string& tableName,
    const std::string& columnName, IndexType type, BST* tree)
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
    active.tree = tree;

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