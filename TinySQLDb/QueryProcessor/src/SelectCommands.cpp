#define _CRT_SECURE_NO_WARNINGS
#include "SelectCommands.h"
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstring>

// Constructor
SelectCommands::SelectCommands(StoredDataManager& dataManager, SystemCatalog& catalog) : dataManager(dataManager), systemCatalog(catalog)
{
	//
}

//Metodo que ejecuta SELECT * FROM <tabla>

void SelectCommands::executeSelect(QueryResult& result, const std::string& statement, const std::string& database)
{

    // extraer el nombre de la tabla despues del FROM
    std::string tableName = this->extractTableName(statement);

    //Si en el nombre de la tabla no hay nada
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: SELECT * FROM <tabla>";
        return;
    }

    // validar que la base de datos y la tabla existan
    if (!this->validateDBTable(database, tableName, result))
    {
        return;
    }

    // obtener la estructura de la tabla del catalog
    Table table = this->systemCatalog.getTable(database, tableName);

    // llenar los nombres de columnas en el resultado
    result.columnCount = table.columnCount;
    for (int i = 0; i < (int)table.columnCount; i++)
    {
        result.columnNames[i] = table.columns[i].name;
    }

    // leer todas las filas activas del archivo binario
    this->readRows(table, result);

    result.success = true;
    //tira la cantidad
    result.message = std::to_string(result.rowCount) + " fila(s) encontrada(s)";
    return;
}

// Extrae el nombre de la tabla del statement despues del FROM
std::string SelectCommands::extractTableName(const std::string& statement)
{
    // convertir a mayusculas para buscar FROM sin importar el caso (esto es opcional)
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion del FROM
    int fromPos = (int)upper.find(" FROM ");
    if (fromPos == -1)
    {
        return "";
    }

    // leer la palabra despues del FROM del statement original (para preservar el caso)
    std::istringstream stream(statement.substr(fromPos + 6));
    std::string tableName;
    stream >> tableName;

    return tableName;
}

// Valida que la base de datos y la tabla existan
bool SelectCommands::validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result)
{
    // verificar que haya una base de datos activa
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return false;
    }

    // verificar que la base de datos exista
    if (!this->systemCatalog.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return false;
    }

    // verificar que la tabla exista
    if (!this->systemCatalog.tableExists(database, tableName))
    {
        result.success = false;
        result.message = "Error: la tabla '" + tableName + "' no existe";
        return false;
    }

    return true;
}

// Convierte los bytes de una columna a string legible
// buffer, es la fila comleta desde el disco como bytes
// col, es la columna que queremos leer 
std::string SelectCommands::deserializeValue(const char* buffer, const Column& col)
{
    if (col.type == TYPE_INTEGER)
    {
        //Copiar 4 bytes del buffer a num y interpretarlos como entero
        int32_t num;
        memcpy(&num, buffer + col.offset, sizeof(int32_t));
        return std::to_string(num);
    }
    else if (col.type == TYPE_DOUBLE)
    {
        // copiar 8 bytes del buffer al num y interpretarlos como double
        double num;
        memcpy(&num, buffer + col.offset, sizeof(double));
        return std::to_string(num);
    }
    else if (col.type == TYPE_VARCHAR)
    {
        // leer el unix timestamp guardado como 8 bytes
        return std::string(buffer + col.offset, col.size);
    }
    else if (col.type == TYPE_DATETIME)
    {
        // leer el unix timestamp guardado como 8 bytes
        int64_t timestamp;
        memcpy(&timestamp, buffer + col.offset, sizeof(int64_t));

        // convertir timestamp unix a string legible YYYY-MM-DD HH:MM:SS
        time_t t = (time_t)timestamp;
        struct tm* timeInfo = localtime(&t);
        char formatted[20];
        strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", timeInfo);
        return std::string(formatted);
    }
       
    return "";
}

// Lee todas las filas activas del archivo binario y llena el resultado
void SelectCommands::readRows(const Table& table, QueryResult& result)
{
    // construir la ruta del archivo binario de la tabla
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";

    // abrir el archivo para lectura binaria
    std::ifstream file(tablePath, std::ios::binary);
    if (!file.is_open())
    {
        result.rowCount = 0;
        return;
    }

    // buffer para leer una fila a la vez
    char* buffer = new char[table.rowSize];
    result.rowCount = 0;

    // leer fila por fila
    while (file.read(buffer, table.rowSize))
    {
        // saltar filas eliminadas (flag = 0)
        if (buffer[0] == 0)
        {
            continue;
        }

        // deserializar cada columna y guardarla en el resultado
        for (int i = 0; i < (int)table.columnCount; i++)
        {
            result.rows[result.rowCount][i] = this->deserializeValue(buffer, table.columns[i]);
        }

        result.rowCount++;

        // no superar el maximo de filas
        if (result.rowCount >= (int)MAX_ROWS)
        {
            break;
        }
    }

    // liberar el buffer
    delete[] buffer;
}