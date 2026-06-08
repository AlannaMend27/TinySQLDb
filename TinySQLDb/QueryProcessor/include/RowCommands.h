#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"

// RowCommmands -> clase que maneja todos los comando relacionados con las filas: insert, select

class RowCommands {
public:
    // constructor
    RowCommands();

    // metodos publicos
    // ejecuta el comando insert into
    QueryResult executeInsert(const std::string& statement, const std::string& database, StoredDataManager& dataManager);

private:
    // Extrae el nombre de la tabla del statement
    std::string extractTableNameForRow(const std::string& statement);

    // Extrae el contenido entre parentesis de VALUES(...)
    std::string extractValuesBody(const std::string& statement);

    // Separa los valores por coma y los guarda en un array, retorna la cantidad de elementos 
    int splitValues(const std::string& body, std::string values[]);
};