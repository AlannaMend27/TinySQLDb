#pragma once
#include <string>
#include "SystemCatalog.h"
#include "Table.h"

/*StoredDataManager, es la clase que actua como intermediario entre el QueryProcessor
y el SystemCatalog, es el que accede a los archivos binarios en el disco
*/
class StoredDataManager {
public:

	// Constructor vacio
	StoredDataManager();

	//metodos relacionados con las bases de datos
	bool createDatabase(const std::string& name);
	bool databaseExists(const std::string& name); 

	//metodos relacionados con tablas (se manda la tabla por que ocupa saber en que db esta)
	bool createTable(const Table& table);
	bool tableExists(const std::string& dbName, const std::string& tableName);

	// metodos relacionados con las filas de la stablas 
	bool insertRow(const std::string& dbName, const std::string& tableName, const std::string values[], int valueCount);
	void serializeRowValues(const Table& table, const std::string values[], char* buffer);

	

private:
	//instancia del system catalog que maneja los archivos binarios
	SystemCatalog catalog;
};
