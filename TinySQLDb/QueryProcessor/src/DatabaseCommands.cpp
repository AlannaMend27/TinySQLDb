#include "DatabaseCommands.h"
#include <sstream>


DatabaseCommands::DatabaseCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    : dataManager(dataManager), systemCatalog(catalog)
{
    //
}

// Ejecuta CREATE DATABASE <nombre>
void DatabaseCommands::executeCreateDatabase(QueryResult& result, const std::string& statement)
{

    // extraer las palabras para saltarse command e instruction y llegar al nombre
    std::istringstream stream(statement);
    std::string command;
    std::string instruction;
    std::string name;
    //esto es como cin >> pero para strings
    stream >> command >> instruction >> name;

    // verificar con system catalog que la base de datos se puede crear
    if (!this->checkCreateDatabseOnCatalog(name))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + name + "' ya existe o el nombre es invalido";
        return;
    }

    // crear la base de datos
    dataManager.createDatabase(name);

    result.success = true;
    result.message = "Base de datos '" + name + "' creada exitosamente";
    return;
}

// verifica si es posible crear la base de datos en el system catalog 
bool DatabaseCommands::checkCreateDatabseOnCatalog(const std::string& name)
{
    // verificar que el nombre no este vacio
    if (name.empty())
    {
        return false;
    }

    // verificar en el system catalog que se 
    // crear el objeto Database con el nombre recibido
    Database db(name);

    // intentar registrar la base de datos en el system catalog
    bool registered = this->systemCatalog.registerDatabase(db);

    // si no se pudo registrar, retornar false
    if (!registered) {
        return false;
    }

    return true;
}


// Ejecuta SET DATABASE <nombre>
// El servidor solo valida que la base de datos exista
// El cliente es quien guarda el contexto localmente
void DatabaseCommands::executeSetDatabase(QueryResult& result, const std::string& statement)
{
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
        return;
    }

    // verificar que la base de datos exista en el sytem catalog
    if (!checkSetDatabseOnCatalog(name))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + name + "' no existe";
        return;
    }

    result.success = true;
    result.message = "Base de datos activa: '" + name + "'";
    return;
}

bool DatabaseCommands::checkSetDatabseOnCatalog(const std::string& name)
{
    return this->systemCatalog.databaseExists(name);
}
