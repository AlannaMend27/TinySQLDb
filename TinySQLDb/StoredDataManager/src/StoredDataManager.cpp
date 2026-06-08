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


// METODOS RELACIONADOS CON LAS FILAS DE LAS TABLAS

// Inserta una fila en la tabla especificada
bool StoredDataManager::insertRow(const std::string& dbName, const std::string& tableName, const std::string values[], int valueCount)
{

    // obtener la estructura de la tabla donde insertar desde el system catalog
    Table table = this->catalog.getTable(dbName, tableName);

    // realizar las validaciones iniciales de los datos recibidos
    bool valid = this->catalog.validationsToInsertRow(table, values, valueCount);

    if (!valid) {
        return false;
    }

    // construir el buffer de bytes que representa la fila completa
    char* buffer = new char[table.rowSize];

    // limpia la memoria e inicializa los bytes del buffer en 0
    memset(buffer, 0, table.rowSize);

    // el byte 0 es el flag — 1 significa fila activa
    buffer[0] = 1;

    // serializar los datos en su posicion correcta dentro del buffer
    // convertir los datos de strings a los que recibe cada columna (int, double, datetime etc..)
    this->serializeRowValues(table, values, buffer);

    // abrir el archivo de la tabla en modo append binario
    std::string tablePath = DATA_PATH + dbName + "/" + tableName + ".bin";
    std::ofstream file(tablePath, std::ios::binary | std::ios::app);

    if (!file.is_open())
    {
        delete[] buffer;
        return false;
    }

    // escribir el buffer completo al final del archivo
    file.write(buffer, table.rowSize);

    // liberar la memoria del buffer
    delete[] buffer;
    return true;
}


// coloca cada valor en su posiion correcta dentro del buffer y convierte el tipo de dato al requerido en la columna
void StoredDataManager::serializeRowValues(const Table& table, const std::string values[], char* buffer)
{
    // serializar cada valor en su posicion correcta dentro del buffer
    for (int i = 0; i < (int)table.columnCount; i++)
    {
        // obtener la columna y el valor de cada posición
        const Column& col = table.columns[i];
        const std::string& value = values[i];

        switch (col.type)
        {
        case TYPE_INTEGER: {
            // convertir el string a entero y copiarlo como bytes
            int32_t num = std::stoi(value);
            memcpy(buffer + col.offset, &num, sizeof(int32_t));
            break;
        }
        case TYPE_DOUBLE: {
            // convertir el string a double y copiarlo como bytes
            double num = std::stod(value);
            memcpy(buffer + col.offset, &num, sizeof(double));
            break;
        }
        case TYPE_VARCHAR: {
            // copiar el string caracter por caracter hasta el tamanio maximo (copia segura de string a buffer)
            strncpy(buffer + col.offset, value.c_str(), col.size);
            break;
        }
        case TYPE_DATETIME: {
            // parsear el string "YYYY-MM-DD HH:MM:SS" a timestamp unix
       
            // usamos struct tm para guardar cada parte de la fecha
            struct tm timeInfo = {};

            // sscanf lee los campos del string con formato de fecha
            sscanf(value.c_str(), "%d-%d-%d %d:%d:%d",
                &timeInfo.tm_year,
                &timeInfo.tm_mon,
                &timeInfo.tm_mday,
                &timeInfo.tm_hour,
                &timeInfo.tm_min,
                &timeInfo.tm_sec);

            // tm_year se cuenta desde 1900 y tm_mon desde 0
            timeInfo.tm_year -= 1900;
            timeInfo.tm_mon -= 1;

            // mktime convierte el struct tm a unix timestamp (la cant de segundos transcurridos desde 1970)
            int64_t timestamp = (int64_t)mktime(&timeInfo);

            // guarda ese numero en el buffer
            memcpy(buffer + col.offset, &timestamp, sizeof(int64_t));
            break;
        }
        default: {
            break;
        }
        }
    }
}


