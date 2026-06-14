#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "table.h"
#include "Commands.h"

// RowCommmands -> clase que maneja todos los comando relacionados con los comandos de insert

class InsertCommands : public Commands {
public:
    // constructor
    InsertCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // metodos publicos
    // ejecuta el comando insert into
    void executeInsert(QueryResult& result, const std::string& statement, const std::string& database);

private:

    // metodos privados

    std::string extractTableNameForRow(const std::string& statement);

    // verificaciones de la base de datos y la tabla desde system catalog
    bool validateAndRegisterTable(const std::string& database, const Table& table, QueryResult& result, const std::string tableName, const std::string values[], const int valueCount);

    // Extrae el contenido entre parentesis de VALUES(...)
    std::string extractValuesBody(const std::string& statement);

    // Separa los valores por coma y los guarda en un array, retorna la cantidad de elementos 
    int splitValues(const std::string& body, std::string values[]);

    // parsear la cadena de datos recibida y colocarla en un buffer para escribirlo luego en archivo
    char* serializeRowValues(const Table& table, const std::string values[], const int rowSize);

};