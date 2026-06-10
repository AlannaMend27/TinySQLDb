#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
// SelectCommands : maneja el comando SELECT y contiene los metodos para ello
class SelectCommands {
public:

    // Constructor
    SelectCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // ejecuta SELECT * FROM <tabla>
    void executeSelect(QueryResult& result, const std::string& statement, const std::string& database);

private:

    // Atributos privados
    StoredDataManager& dataManager;
    SystemCatalog& systemCatalog;

    // extrae el nombre de la tabla despues del FROM
   
    // Notaaa: Si ves los nombres de varios metodos son iguales, 
    // la idea es la misma pero cada uno parsea para lo que cada uno ocupa, 
    // lo podemmos dejar asi con el mismo nombre o poner, extractTableNameForSelect (obvio uno mas corto)
    std::string extractTableName(const std::string& statement);

    // valida que la base de datos y la tabla existan
    bool validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result);

    // convierte los bytes de una columna a string legible
    std::string deserializeValue(const char* buffer, const Column& col);

    // lee todas las filas activas del archivo binario y llena el resultado
    void readRows(const Table& table, QueryResult& result);
};