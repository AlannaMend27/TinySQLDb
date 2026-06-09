#define _CRT_SECURE_NO_WARNINGS
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
void StoredDataManager::createDatabase(const std::string& name) {

    // crear la carpeta de la base de datos en disco
    std::string dbFolderPath = DATA_PATH + name + "/";
    std::filesystem::create_directories(dbFolderPath);

}

//METODOS RELACIONADOS CON TABLAS

// Crea una tabla nueva en el system catalog y su archivo binario en disco
void StoredDataManager::createTable(const Table& table)
{
    // crear el archivo binario vacio de la tabla en disco
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";
    std::ofstream file(tablePath, std::ios::binary);
}


// METODOS RELACIONADOS CON LAS FILAS DE LAS TABLAS

// escribe una fila ya serializada en el archivo binario de la tabla
bool StoredDataManager::insertRow(const std::string& dbName, const std::string& tableName, char* buffer, uint32_t rowSize)
{
    // abrir el archivo de la tabla en modo append binario
    std::string tablePath = DATA_PATH + dbName + "/" + tableName + ".bin";
    std::ofstream file(tablePath, std::ios::binary | std::ios::app);

    // verificar si el archivo se abrio correctamente
    if (!file.is_open())
    {
        return false;
    }

    // escribir el buffer completo al final del archivo
    file.write(buffer, rowSize);
    return true;
}



