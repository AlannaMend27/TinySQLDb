#define _CRT_SECURE_NO_WARNINGS
#include "QueryProcessor.h"
#include <algorithm>
#include <sstream>
#include <chrono>


// Constructor vacio
QueryProcessor::QueryProcessor()
{
    this->dataManager = StoredDataManager();
}

// Recibe una sentencia SQL, identifica el comando y lo ejecuta
QueryResult QueryProcessor::execute(const std::string& statement, const std::string& database)
{
    // registrar tiempo de inicio
    auto start = std::chrono::high_resolution_clock::now();

    // limpiar la sentencia, para quitarle los puntos y coma
    std::string clean = this->cleanStatement(statement);

    // convertir a mayusculas 
    std::string upper = clean;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // leer la instruccion y que crear para saber que ejecutar
    std::istringstream stream(upper);
    std::string instruction;
    std::string category;

    //esto es como un std::cin >> pero lee de string
    stream >> instruction >> category;

    //Identificar el comando
    CommandType command = this->identifyCommand(instruction, category);

    //Guardamos todo en el struct
    QueryResult result;

    switch (command)
    {
    case COMMAND_CREATE_DATABASE:
        result = this->executeCreateDatabase(clean);
        break;
    case COMMAND_SET_DATABASE:
        result = this->executeSetDatabase(clean);
        break;
    default:
        result.success = false;
        result.message = "Sentencia no reconocida";
        break;
    }

    // calcular tiempo transcurrido en milisegundos
    auto end = std::chrono::high_resolution_clock::now();
    result.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

// Metodo que ejecuta CREATE DATABASE
QueryResult QueryProcessor::executeCreateDatabase(const std::string& statement)
{
    QueryResult result;

    // extraer las tres instrucciones, para saltarse command y instruction, y llegar al nombre 
    std::istringstream stream(statement);
    std::string command;
    std::string instruction;
    std::string name;
    stream >> command >> instruction >> name;

    // verificar que el nombre no este vacio
    if (name.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use por ejemplo: CREATE DATABASE <nombre>";
        return result;
    }

    // intentar crear la base de datos
    bool created = this->dataManager.createDatabase(name);

    //No se pudo
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
QueryResult QueryProcessor::executeSetDatabase(const std::string& statement)
{
    QueryResult result;


    // extraer las palabras para saltarse command e instruction, y llegar al nombre
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
    bool exists = this->dataManager.databaseExists(name);

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

// Limpia la sentencia de espacios sobrantes y el punto y coma final
std::string QueryProcessor::cleanStatement(const std::string& statement)
{
    std::string clean = statement;

    // quitar espacios y tabs del inicio
    while (!clean.empty() && (clean.front() == ' ' || clean.front() == '\t'))
    {
        clean.erase(clean.begin()); //borra un caracter en esa posicion
    }

    // quitar espacios, tabs ('t') y punto y coma del final
    while (!clean.empty() && (clean.back() == ' ' || clean.back() == '\t' || clean.back() == ';'))
    {
        clean.pop_back(); //esto quita el ultimo caracter de un string
    }

    return clean;
}

// Identifica el tipo de comando a partir de la instruccion y la categoria
CommandType QueryProcessor::identifyCommand(const std::string& instruction, const std::string& category)
{
    if (instruction == "CREATE" && category == "DATABASE")
    {
        return COMMAND_CREATE_DATABASE;
    }
    if (instruction == "SET" && category == "DATABASE")
    {
        return COMMAND_SET_DATABASE;
    }

    //En caso de que no sea correcto 
    return COMMAND_UNKNOWN;
}