#pragma once
#include <string>
#include "SystemCatalog.h"
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "DatabaseCommands.h"
#include "TableCommands.h"
#include "InsertCommands.h"
#include "SelectCommands.h"

// QueryProcessor: recibe sentencias SQL, identifica el comando,
// valida la sintaxis y coordina que ejecutar con StoredDataManager
 
//aqui descomentamos segun hacemos
enum CommandType {
    COMMAND_CREATE_DATABASE,
    COMMAND_SET_DATABASE,
	COMMAND_CREATE_TABLE,
	COMMAND_INSERT,
	COMMAND_SELECT,
	/*
    COMMAND_DROP_TABLE,,
    COMMAND_UPDATE,
    COMMAND_DELETE,
    COMMAND_CREATE_INDEX,
	*/
    COMMAND_UNKNOWN
};

class QueryProcessor {
public:

	//constructor
	QueryProcessor();

	//recibe una sentencia SQL y ejecuta, statement es la sentencia, database es la base de datos activa
	void execute(QueryResult& result, const std::string& statement, const std::string& database);

private:

	//instancia de StoredDataManager para acceder a los datos del disco
	StoredDataManager dataManager;

	// instancia del system catalog para realizar las validaciones necesarias
	SystemCatalog systemCatalog;

	//El administrador de los comandos para cada uno 
	DatabaseCommands databaseCommands;
	TableCommands tableCommands;
	InsertCommands InsertCommands;
	SelectCommands selectCommands;

	//Metodos privados

	CommandType identifyCommand(const std::string& instruction, const std::string& category);

	// limpia los puntos y coma y espacios sobrantes
	std::string cleanStatement(const std::string& statement);
};
