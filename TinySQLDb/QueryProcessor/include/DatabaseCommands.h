#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"

// DatabaseCommands maneja todos los comandos SQL relacionados con bases de datos

class DatabaseCommands {
public:

    // Constructor 
    DatabaseCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // ejecuta CREATE DATABASE <nombre>
    QueryResult executeCreateDatabase(const std::string& statement);
    bool checkCreateDatabseOnCatalog(const std::string& name);

    // ejecuta SET DATABASE <nombre>
    QueryResult executeSetDatabase(const std::string& statement);
    bool checkSetDatabseOnCatalog(const std::string& name);

private:
    // Atributos privados 
    StoredDataManager& dataManager;
    SystemCatalog& systemCatalog;

};