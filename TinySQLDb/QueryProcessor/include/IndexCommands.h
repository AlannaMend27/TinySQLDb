#pragma once
#include <string>
#include "QueryResult.h"
#include "Commands.h"
#include "IndexManager.h"

// IndexCommands -> maneja el comando CREATE INDEX
// crea indices BST o BTREE sobre columnas de una tabla

class IndexCommands : public Commands {
public:
    // Constructor 
    IndexCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager);
    // ejecuta CREATE INDEX <nombre> ON <tabla>(<columna>) OF TYPE <tipo>
    void executeCreateIndex(QueryResult& result, const std::string& statement, const std::string& database);

private:

    //gestor de indices en memoria
    IndexManager& indexManager;

    std::string extractIndexName(const std::string& statement);
    std::string extractTableName(const std::string& statement);
    std::string extractColumnName(const std::string& statement);
    IndexType extractIndexType(const std::string& statement);
    bool columnHasNoDuplicates(const Table& table, const Column& col);
};