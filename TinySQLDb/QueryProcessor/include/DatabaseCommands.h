#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"

// DatabaseCommands maneja todos los comandos SQL relacionados con bases de datos

class DatabaseCommands {
public:

    // Constructor 
    DatabaseCommands();

    // ejecuta CREATE DATABASE <nombre>
    QueryResult executeCreateDatabase(const std::string& statement, StoredDataManager& dataManager);

    // ejecuta SET DATABASE <nombre>
    QueryResult executeSetDatabase(const std::string& statement, StoredDataManager& dataManager);
};