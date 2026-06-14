#pragma once

#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Commands.h"

// Delete commands -> clase que maneja el comando delete de la base de datos
// actualiza las columnas de una fila o filas en una tabla

class DeleteCommands : public Commands {
public:

    // Constructor 
    DeleteCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // ejecuta el comando delete
    void executeDelete(QueryResult& result, const std::string& statement, const std::string& database);

private:

    // extrae el nombre de la tabla
    std::string extractTableName(const std::string& statement);

    int deleteMatchingRows(const Table& table, const std::string& whereColumn,
        const std::string& whereOperator, const std::string& whereValue, bool hasWhere);


};
