#define _CRT_SECURE_NO_WARNINGS
#include "DropCommands.h"
#include <sstream>

// Constructor
DropCommands::DropCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    : dataManager(dataManager), systemCatalog(catalog)
{
    //
}

// extrae el nombre de la tabla
// DROP TABLE Estudiante → "Estudiante"
std::string DropCommands::extractTableName(const std::string& statement)
{
    // creamos variables para almacenar los textos
    std::istringstream stream(statement);
    std::string drop;
    std::string table;
    std::string name;

    // saltamos DROP y TABLE y leemos el nombre
    stream >> drop >> table >> name;
    return name;
}

// valida que la base de datos y la tabla existan
bool DropCommands::validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result)
{
    // si no hay nombre de la base de datos, retornar falso
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return false;
    }

    // si la base de datos no existe, retornar falso
    if (!this->systemCatalog.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return false;
    }
    
    // si la tabla de la statemente no existe, retornar falso
    if (!this->systemCatalog.tableExists(database, tableName))
    {
        result.success = false;
        result.message = "Error: la tabla '" + tableName + "' no existe";
        return false;
    }

    return true;
}

// Ejecuta DROP TABLE <tabla>
void DropCommands::executeDrop(QueryResult& result, const std::string& statement, const std::string& database)
{
    // extraer el nombre de la tabla
    std::string tableName = this->extractTableName(statement);

    // verificar que el nombre de la tabla no este vacio
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: DROP TABLE <tabla>";
        return;
    }

    // validar que la base de datos y la tabla existan
    if (!this->validateDBTable(database, tableName, result))
    {
        return;
    }

    // eliminar el archivo .bin del disco
    bool fileDeleted = this->dataManager.deleteTableFile(database, tableName);

    //verificar si fue posible eliminarla
    if (!fileDeleted)
    {
        result.success = false;
        result.message = "Error: no se pudo eliminar el archivo de la tabla '" + tableName + "'";
        return;
    }

    // eliminar la metadata del system catalog
    this->systemCatalog.unregisterTable(database, tableName);

    result.success = true;
    result.message = "Tabla '" + tableName + "' eliminada exitosamente";
}