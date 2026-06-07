#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "DatabaseCommands.h"
#include "TableCommands.h"

// QueryProcessor: recibe sentencias SQL, identifica el comando,
// valida la sintaxis y coordina que ejecutar con StoredDataManager

//aqui descomentamos segun hacemos
enum CommandType {
    COMMAND_CREATE_DATABASE,
    COMMAND_SET_DATABASE,
	COMMAND_CREATE_TABLE,
	/*
    COMMAND_DROP_TABLE,
    COMMAND_INSERT,
    COMMAND_SELECT,
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
	QueryResult execute(const std::string& statement, const std::string& database);

private:

	//instancia de StoredDataManager para acceder a los datos del disco
	StoredDataManager dataManager;

	//El administrador de los comandos para cada uno (la refactorizacion)
	DatabaseCommands databaseCommands;
	TableCommands tableCommands;

	//Metodos privados

	CommandType identifyCommand(const std::string& instruction, const std::string& category);

	// limpia los puntos y coma y espacios sobrantes
	std::string cleanStatement(const std::string& statement);
};
