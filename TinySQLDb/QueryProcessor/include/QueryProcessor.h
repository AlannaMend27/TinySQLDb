#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"

// QueryProcessor -> recibe sentencias SQL, identifica el comando,
// valida la sintaxis y coordina que ejecutar con StoredDataManager

//aqui descomentamos segun hacemos
enum CommandType {
    COMMAND_CREATE_DATABASE,
    COMMAND_SET_DATABASE,
    /*
	COMMAND_CREATE_TABLE,
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
	//construcotr
	QueryProcessor();

	//recibe una sentencia SQL y ejecuta, statement es la sentencia, database es la base de datos activa
	QueryResult execute(const std::string& statement, const std::string& database);

private:

	//instancia de StoredDataManager para acceder a los datos del disco
	StoredDataManager dataManager;

	//Metodos privados

	CommandType identifyCommand(const std::string& instruction, const std::string& category);

	// metodos privados, uno por cada sentencia SQL soportada hoy
	QueryResult executeCreateDatabase(const std::string& statement);
	QueryResult executeSetDatabase(const std::string& statement);

	// limpia el puntos y coma y espacios sobrantes
	std::string cleanStatement(const std::string& statement);
};
