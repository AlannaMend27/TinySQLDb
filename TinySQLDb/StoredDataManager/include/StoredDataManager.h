#pragma once

#include <string>
#include "SystemCatalog.h"
#include "Database.h"

/*StoredDataManager, es la clase que actua como intermidiario entre el QueryProcessor
y el SystemCatalog, es el que accese a los archivos binarios en el disco
*/
class StoredDataManager {
public:

	// Constructor vacio
	StoredDataManager();
	// Constructor completo
	explicit StoredDataManager(const std::string& catalogPath);

	//metodos relacionados con las bases de datos
	bool createDatabase(const std::string& name);
	bool databaseExists(const std::string& name);

private:
	//instancia del system catalog que maneja los archivos binarios
	SystemCatalog catalog;
};
