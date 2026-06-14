#pragma once

#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"

// Delete commands -> clase que maneja el comando delete de la base de datos
// actualiza las columnas de una fila o filas en una tabla

class DeleteCommands {
public:

    // Constructor 
    DeleteCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // ejecuta el comando delete
    void executeDelete(QueryResult& result, const std::string& statement, const std::string& database);

private:

    StoredDataManager& dataManager;
    SystemCatalog& systemCatalog;

    // extrae el nombre de la tabla
    std::string extractTableName(const std::string& statement);

    // valida que la base de datos y la tabla existan
    bool validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result);

    // extrae la condicion WHERE si existe
    bool parseWhere(const std::string& statement, std::string& whereColumn, std::string& whereOperator, std::string& whereValue);

    // desealización de un dato de una columna
    std::string deserializeValue(const char* buffer, const Column& col);

    // verifica donde hay un match en filas para cambiar el valor 
    bool rowMatchesWhere(const char* buffer, const Table& table, const std::string& whereColumn,
        const std::string& whereOperator, const std::string& whereValue);

    int deleteMatchingRows(const Table& table, const std::string& whereColumn,
        const std::string& whereOperator, const std::string& whereValue, bool hasWhere);


};
