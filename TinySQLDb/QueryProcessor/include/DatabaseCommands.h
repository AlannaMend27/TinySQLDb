#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Commands.h"

// DatabaseCommands maneja todos los comandos SQL relacionados con bases de datos

class DatabaseCommands : public Commands {
public:

    // Constructor 
    DatabaseCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // ejecuta CREATE DATABASE <nombre>
    void executeCreateDatabase(QueryResult& result, const std::string& statement);
    bool checkCreateDatabseOnCatalog(const std::string& name);

    // ejecuta SET DATABASE <nombre>
    void executeSetDatabase(QueryResult& result, const std::string& statement);
    bool checkSetDatabseOnCatalog(const std::string& name);

};