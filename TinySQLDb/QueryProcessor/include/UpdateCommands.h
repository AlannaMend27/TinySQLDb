#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Commands.h"
#include "IndexManager.h"

// UpdateCommands -> clase que maneja el comando update de la base de datos
// actualiza las columnas de una fila o filas en una tabla

class UpdateCommands : public Commands {
public:

    // Constructor 
    UpdateCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager);

    // ejecuta el comando update
    void executeUpdate(QueryResult& result, const std::string& statement, const std::string& database);

private:

    IndexManager& indexManager;

    // extrae el nombre de la tabla
    std::string extractTableName(const std::string& statement);

    int updateWithIndex(const Table& table, const Column& setCol, const std::string& setValue, const std::string& whereColumn, const std::string& whereValue, char* allRows);
    int updateSequential(const Table& table, const Column& setCol, const std::string& setValue, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue, bool hasWhere, char* allRows, int rowCount);

    // extrae la columna y el valor del SET
    bool parseSet(const std::string& statement,std::string& setColumn, std::string& setValue, QueryResult& result);

    int updateMatchingRows(const Table& table, const Column& setCol, const std::string& setValue,
                           const std::string& whereColumn, const std::string& whereOperator,
                           const std::string& whereValue, bool hasWhere);


};
