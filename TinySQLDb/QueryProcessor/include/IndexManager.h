#pragma once
#include <string>
#include "BST.h"
#include "SystemCatalog.h"
#include "StoredDataManager.h"
#include "Records.h"
#include "Commands.h"
#include "BTree.h"

// cantidad maxima de indices activos en memoria 
const int MAX_INDEXES = 50;

// ActiveIndex representa un indice activo en memoria
struct ActiveIndex {
    std::string tableName; 
    std::string columnName;
    std::string indexName; 
    // BST o BTREE
    IndexType type;  
    BST* BST;      
    BTree* bTree; 

    // Constructor vacio
    ActiveIndex()
    {
        this->BST = nullptr;
        this->bTree = nullptr;
        this->type = INDEX_BST;
        
    }
};

// IndexManager, administra todos los indices activos en memoria
class IndexManager {
public:

    //Constructor y destructor
    IndexManager();
    ~IndexManager();

    // reconstruye todos los indices al iniciar el servidor
    void loadFromCatalog(SystemCatalog& catalog, StoredDataManager& dataManager);

    bool addIndex(const std::string& indexName, const std::string& tableName,const std::string& columnName, IndexType type, BST* bstTree, BTree* btreeTree);
    ActiveIndex* getIndex(const std::string& tableName, const std::string& columnName);
    bool hasIndex(const std::string& tableName, const std::string& columnName);

private:
    // arreglo de indices activos
    ActiveIndex indexes[MAX_INDEXES];
    int indexCount;
};