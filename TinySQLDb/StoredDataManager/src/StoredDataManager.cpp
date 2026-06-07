#include "StoredDataManager.h"
#include <filesystem>
#include <fstream>


// Constructor vacio usa la ruta por defecto definida en Records.h
StoredDataManager::StoredDataManager()
{
    this->catalog = SystemCatalog(CATALOG_PATH);
}


//METODOS RELACIONADOS CON BASES DE DATOS 

// Crea una base de datos nueva en el system catalog y su carpeta en disco
bool StoredDataManager::createDatabase(const std::string& name) {

    // crear el objeto Database con el nombre recibido
    Database db(name);

    // intentar registrar la base de datos en el system catalog
    bool registered = this->catalog.registerDatabase(db);

    // si no se pudo registrar, retornar false
    if (!registered) {
        return false;
    }

    // crear la carpeta de la base de datos en disco
    std::string dbFolderPath = DATA_PATH + name + "/";
    std::filesystem::create_directories(dbFolderPath);

    return true;
}

// Verifica si una base de datos existe en el system catalog
bool StoredDataManager::databaseExists(const std::string& name) {

    // delegar la verificacion al system catalog
    return this->catalog.databaseExists(name);
}

//METODOS RELACIONADOS CON TABLAS

// Crea una tabla nueva en el system catalog y su archivo binario en disco
bool StoredDataManager::createTable(const Table& table)
{
    // registrar la tabla y sus columnas en el system catalog
    bool registered = this->catalog.registerTable(table);

    // si no se pudo registrar
    if (!registered)
    {
        return false;
    }

    // crear el archivo binario vacio de la tabla en disco
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";
    std::ofstream file(tablePath, std::ios::binary);

    return true;
}

// Verifica si una tabla existe en una base de datos especifica
bool StoredDataManager::tableExists(const std::string& dbName, const std::string& tableName)
{
    // delegar la verificacion al system catalog
    return this->catalog.tableExists(dbName, tableName);
}