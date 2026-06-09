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
	void createDatabase(const std::string& name);

	//metodos relacionados con tablas (se manda la tabla por que ocupa saber en que db esta)
	void createTable(const Table& table);

	// metodos relacionados con las filas de la stablas 
	bool insertRow(const std::string& dbName, const std::string& tableName, char* buffer, uint32_t rowSize);
	

private:
	//instancia del system catalog que maneja los archivos binarios
	SystemCatalog catalog;
};
