#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Commands.h"
#include "IndexManager.h"

// Delete commands -> clase que maneja el comando delete de la base de datos
// actualiza las columnas de una fila o filas en una tabla

class DeleteCommands : public Commands {
public:

    // Constructor 
    DeleteCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager);

    // ejecuta el comando delete
    void executeDelete(QueryResult& result, const std::string& statement, const std::string& database);

private:

    IndexManager& indexManager;

    // extrae el nombre de la tabla
    std::string extractTableName(const std::string& statement);
    
    // elimina una fila usando el indice para busqueda directa
    int deleteWithIndex(const Table& table, const std::string& whereColumn, const std::string& whereValue, char* allRows);

    // elimina filas usando busqueda secuencial
    int deleteSequential(const Table& table, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue, bool hasWhere, char* allRows, int rowCount);

    int deleteMatchingRows(const Table& table, const std::string& whereColumn,
        const std::string& whereOperator, const std::string& whereValue, bool hasWhere);


};
