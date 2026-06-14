#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"

// DropCommands -> se encarga de 

class DropCommands {
public:
    DropCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // Ejecuta DROP TABLE <tabla>
    void executeDrop(QueryResult& result, const std::string& statement, const std::string& database);

private:
    StoredDataManager& dataManager;
    SystemCatalog& systemCatalog;

    // extrae el nombre de la tabla
    // DROP TABLE Estudiante → "Estudiante"
    std::string extractTableName(const std::string& statement);

    // valida que la base de datos y la tabla existan
    bool validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result);
};