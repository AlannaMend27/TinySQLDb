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

// metodos insert row

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

// metodos update table

// lee todas las filas de una tabla y las retorna en un buffer
char* StoredDataManager::readAllRows(const Table& table, int& rowCount)
{
    // obteiene en path del archivo que contiene la tabla
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";
    std::ifstream file(tablePath, std::ios::binary | std::ios::ate);

    // si el archivo no se pudo abrir, se devuelve nullptr
    if (!file.is_open())
    {
        rowCount = 0;
        return nullptr;
    }

    // obtener la cantidad de bytes que tiene el archivo
    long fileSize = (long)file.tellg();

    // calcular la cantidad de filas en base al tamanio de bytes de las filas de la tabla
    rowCount = fileSize / table.rowSize;

    // en caso de no haber filas, se retorna nullptr
    if (rowCount == 0)
    {
        return nullptr;
    }

    // volver al inicio para leer todo
    file.seekg(0, std::ios::beg);

    // colocar los datos de la tabla en el buffer
    char* buffer = new char[fileSize];
    file.read(buffer, fileSize);

    // retornar el buffer para realizar las actualizaciones de UPDATE
    return buffer;
}

// escribe una fila especifica en su posicion dentro del archivo
bool StoredDataManager::writeRowAt(const Table& table, int rowIndex, char* buffer)
{
    // obtener el path de tabla y abrir el archivo
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";
    std::fstream file(tablePath, std::ios::binary | std::ios::in | std::ios::out);

    // verifcar que el archivo se abrio correctamente
    if (!file.is_open())
    {
        return false;
    }

    // calcular la posicion exacta de esa fila en el archivo
    std::streamoff position = (std::streamoff)rowIndex * table.rowSize;

    // escribir el buffer en la posicion dada
    file.seekp(position, std::ios::beg);
    file.write(buffer, table.rowSize);

    return true;
}

// verifica si la tabla no tiene filas activas
bool StoredDataManager::isTableEmpty(const std::string& dbName,const std::string& tableName)
{
    Table table = this->catalog.getTable(dbName, tableName);
    if (!table.isValid()) return true;

    int rowCount = 0;
    char* allRows = this->readAllRows(table, rowCount);

    if (allRows == nullptr) return true;

    // recorrer las filas buscando alguna activa
    bool empty = true;
    for (int i = 0; i < rowCount; i++)
    {
        char* row = allRows + (i * table.rowSize);
        if (row[0] == 1)
        {
            // encontramos una fila activa — la tabla no esta vacia
            empty = false;
            break;
        }
    }

    delete[] allRows;
    return empty;
}

// elimina el archivo .bin de la tabla del disco
bool StoredDataManager::deleteTableFile(const std::string& dbName, const std::string& tableName)
{
    // obtener el path de la tabla
    std::string tablePath = DATA_PATH + dbName + "/" + tableName + ".bin";
    std::filesystem::path path(tablePath);

    // verificar que el archivo existe antes de intentar borrarlo
    if (!std::filesystem::exists(path))
    {
        return false;
    }

    // remove elimina el archivo y devuelve true si lo logro
    return std::filesystem::remove(path);
}



