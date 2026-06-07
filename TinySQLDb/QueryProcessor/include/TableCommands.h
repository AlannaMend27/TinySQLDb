#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"

// TableCommands: maneja todos los comandos SQL relacionados con tablas
// CREATE TABLE y DROP TABLE depsues

class TableCommands {
public:

    // Constructor vacio
    TableCommands();

    // ejecuta CREATE TABLE
    QueryResult executeCreateTable(const std::string& statement, const std::string& database, StoredDataManager& dataManager);

private:

    // valida que haya una base de datos activa y que exista
    bool validateContext(const std::string& database, StoredDataManager& dataManager, QueryResult& result);

    // extrae el nombre de la tabla 
    std::string extractTableName(const std::string& statement);

    // extrae el contenido entre parentesis
    std::string extractBody(const std::string& statement);

    // separa el cuerpo en definiciones de columna por coma
    int splitColumns(const std::string& body, std::string colDefs[]);

    // convierte a un objeto Column
    bool parseColumn(const std::string& colDef, const std::string& tableName, int position, int offset, Column& column, QueryResult& result);

    // asigna el type y su tamanio
    bool resolveType(const std::string& upperType, ColumnType& type, int& size, QueryResult& result);
};
