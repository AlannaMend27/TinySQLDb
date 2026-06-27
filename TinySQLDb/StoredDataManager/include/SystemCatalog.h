#pragma once
#include <string>
#include <filesystem>
#include "Database.h"
#include "Table.h"
#include "Column.h"
#include "Index.h"
#include "Records.h"

// SystemCatalog -> clase que se encarga de leer y escribir los archivos binarios en el catalog

// Rutas base del sistema de archivos

const std::string BASE_PATH = "C:/TinySQLDb/";
const std::string CATALOG_PATH = BASE_PATH + "SystemCatalog/";
const std::string DATA_PATH = BASE_PATH + "data/";

class SystemCatalog {
public:

    // Constructor vacio
    SystemCatalog();

    // Constructor completo
    explicit SystemCatalog(const std::string& catalogPath);

    // metodos publicos
    void initialize();

    // metodos relaciondos a bases de datos
    bool registerDatabase(Database& db);
    bool databaseExists(const std::string& name) const;
    Database* getAllDatabases(int& count) const;

    // metodos relacionados a tablas
    bool registerTable(const Table& table);
    bool tableExists(const std::string& dbName, const std::string& tableName) const;
    bool validationsToInsertRow(const Table table, const std::string values[], int valueCount);
    Table getTable(const std::string& dbName, const std::string& tableName) const;
    Table* getTablesForDatabase(const std::string& dbName) const;
    bool unregisterTable(const std::string& dbName, const std::string& tableName);
    std::string getDatabaseForTable(const std::string& tableName) const;
    Table* getAllTables(int& count) const;
    Column* getAllColumns(int& count) const;
    
    // metodos relacionados con indices
    bool registerIndex(const Index& index);
    Index* getAllIndexes(int& count) const;
    Index getIndexForColumn(const std::string& tableName, const std::string& columnName) const;
    bool unregisterIndex(const std::string& indexName);

private:

    // Atributos privados
    std::string path; 

    // Métodos privados usados internamente por system catalog
    std::filesystem::path buildPath(const std::string& fileName) const;
    void createFileIfNotExists(const std::filesystem::path& filePath) const;
    Column recordToColumn(const ColumnRecord& record) const;
    Index recordToIndex(const IndexRecord& record) const;
};