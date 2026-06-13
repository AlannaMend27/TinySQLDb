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

	// lee las filas de una tabla y las almacena en un buffer
	char* readAllRows(const Table& table, int& rowCount);

	// escribe una fila especifica en una posicion de archivo
	bool writeRowAt(const Table& table, int rowIndex, char* buffer);
	

private:
	//instancia del system catalog que maneja los archivos binarios
	SystemCatalog catalog;
};
