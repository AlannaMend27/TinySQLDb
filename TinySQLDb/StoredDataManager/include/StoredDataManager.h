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
	bool deleteTableFile(const std::string& dbName, const std::string& tableName);
	bool isTableEmpty(const std::string& dbName, const std::string& tableName);
	// metodos relacionados con las filas de la stablas 
	bool insertRow(const std::string& dbName, const std::string& tableName, char* buffer, uint32_t rowSize);
	char* readAllRows(const Table& table, int& rowCount);
	bool writeRowAt(const Table& table, int rowIndex, char* buffer);

	// aplica XOR al buffer para encriptar o desencriptar
	void encryptBuffer(char* buffer, uint32_t size);
	

private:
	//instancia del system catalog que maneja los archivos binarios
	SystemCatalog catalog;

	// clave de encriptacion fija 7 en hexa
	const char KEY = 0x7;


};
