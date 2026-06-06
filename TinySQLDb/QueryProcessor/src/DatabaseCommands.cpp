#define _CRT_SECURE_NO_WARNINGS
#include "DatabaseCommands.h"
#include <sstream>

// Constructor vacio
DatabaseCommands::DatabaseCommands()
{
}

// Ejecuta CREATE DATABASE <nombre>
QueryResult DatabaseCommands::executeCreateDatabase(const std::string& statement, StoredDataManager& dataManager)
{
    QueryResult result;

    // extraer las palabras para saltarse command e instruction y llegar al nombre
    std::istringstream stream(statement);
    std::string command;
    std::string instruction;
    std::string name;
    stream >> command >> instruction >> name;

    // verificar que el nombre no este vacio
    if (name.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: CREATE DATABASE <nombre>";
        return result;
    }

    // intentar crear la base de datos
    bool created = dataManager.createDatabase(name);

    if (!created)
    {
        result.success = false;
        result.message = "Error: la base de datos '" + name + "' ya existe o el nombre es invalido";
        return result;
    }

    result.success = true;
    result.message = "Base de datos '" + name + "' creada exitosamente";
    return result;
}

// Ejecuta SET DATABASE <nombre>
// El servidor solo valida que la base de datos exista
// El cliente es quien guarda el contexto localmente
QueryResult DatabaseCommands::executeSetDatabase(const std::string& statement, StoredDataManager& dataManager)
{
    QueryResult result;

    // extraer las palabras para saltarse command e instruction y llegar al nombre
    std::istringstream stream(statement);
    std::string command;
    std::string instruction;
    std::string name;
    stream >> command >> instruction >> name;

    // verificar que el nombre no este vacio
    if (name.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: SET DATABASE <nombre>";
        return result;
    }

    // verificar que la base de datos exista
    bool exists = dataManager.databaseExists(name);

    if (!exists)
    {
        result.success = false;
        result.message = "Error: la base de datos '" + name + "' no existe";
        return result;
    }

    result.success = true;
    result.message = "Base de datos activa: '" + name + "'";
    return result;
}