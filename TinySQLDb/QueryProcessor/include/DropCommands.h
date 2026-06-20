#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Commands.h"

// DropCommands -> se encarga de                                         

class DropCommands : public Commands {
public:
    DropCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // Ejecuta DROP TABLE <tabla>
    void executeDrop(QueryResult& result, const std::string& statement, const std::string& database);

private:

    // extrae el nombre de la tabla
    // DROP TABLE Estudiante → "Estudiante"
    std::string extractTableName(const std::string& statement);

};