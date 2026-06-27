#include "QueryProcessor.h"
#include <algorithm>
#include <sstream>
#include <chrono>

// Constructor del query processor que conecta todas las clases necesarias
QueryProcessor::QueryProcessor()
    : dataManager(),
    systemCatalog(),
    databaseCommands(dataManager, systemCatalog),
    tableCommands(dataManager, systemCatalog),
    insertCommands(dataManager, systemCatalog, indexManager),
    selectCommands(dataManager, systemCatalog, indexManager),
    updateCommands(dataManager, systemCatalog, indexManager),
    deleteCommands(dataManager, systemCatalog, indexManager),
    dropCommands(dataManager, systemCatalog),
    indexCommands(dataManager, systemCatalog, indexManager)
{
    // reconstruir indices existentes al iniciar el servidor
    this->indexManager.loadFromCatalog(this->systemCatalog, this->dataManager);
}

// Recibe una sentencia SQL, identifica el comando y lo ejecuta
void QueryProcessor::execute(QueryResult& result, const std::string& statement, const std::string& database)
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

    switch (command)
    {
    case COMMAND_CREATE_DATABASE:
        this->databaseCommands.executeCreateDatabase(result, clean);
        break;
    case COMMAND_SET_DATABASE:
        this->databaseCommands.executeSetDatabase(result, clean);
        break;
    case COMMAND_CREATE_TABLE:
        this->tableCommands.executeCreateTable(result, clean, database);
        break;
    case COMMAND_INSERT:
        this->insertCommands.executeInsert(result, clean, database);
        break;
    case COMMAND_SELECT:
        this->selectCommands.executeSelect(result, clean, database);
        break;
    case COMMAND_UPDATE:
        this->updateCommands.executeUpdate(result, clean, database);
        break;
    case COMMAND_DELETE:
        this->deleteCommands.executeDelete(result, clean, database);
        break;
    case COMMAND_DROP_TABLE:
        this->dropCommands.executeDrop(result, clean, database);
        break;
    case COMMAND_CREATE_INDEX:
        this->indexCommands.executeCreateIndex(result, clean, database);
        break;
    default:
        result.success = false;
        result.message = "Sentencia no reconocida";
        break;
    }

    // calcular tiempo transcurrido en milisegundos
    auto end = std::chrono::high_resolution_clock::now();
    result.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

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
    if (instruction == "CREATE" && category == "TABLE")
    {
        return COMMAND_CREATE_TABLE;
    }
    if (instruction == "INSERT" && category == "INTO")
    {
        return COMMAND_INSERT;
    }
    if (instruction == "SELECT")
    {
        return COMMAND_SELECT;
    }
    if (instruction == "UPDATE")
    {
        return COMMAND_UPDATE;
    }
    if (instruction == "DELETE" && category == "FROM")
    {
        return COMMAND_DELETE;
    }
    if (instruction == "DROP" && category == "TABLE")
    {
        return COMMAND_DROP_TABLE;
    }
    if (instruction == "CREATE" && category == "INDEX")
    {
        return COMMAND_CREATE_INDEX;
    }

    //En caso de que no sea correcto 
    return COMMAND_UNKNOWN;
}