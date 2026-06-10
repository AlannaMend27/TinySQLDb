#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Column.h"
#include "SortAlgorithms.h"

// SelectCommands : maneja el comando SELECT y contiene los metodos para ello
class SelectCommands { 
public:

    // Constructor
    SelectCommands(StoredDataManager& dataManager, SystemCatalog& catalog);

    // ejecuta SELECT * FROM <tabla>
    void executeSelect(QueryResult& result, const std::string& statement, const std::string& database);

private:

    // Atributos privados
    StoredDataManager& dataManager;
    SystemCatalog& systemCatalog;
    SortAlgorithms quickSorter;

    //METODOS PARA VALIDACIONES

    // valida que la base de datos y la tabla existan
    bool validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result);

    // verifica que las columnas seleccionadas existan en la tabla
    bool validateColumns(const Table& table, const std::string selectedCols[], int selectedCount, QueryResult& result);

    //METODOS DE PARSEO GENERAL

    // extrae el nombre de la tabla despues del FROM
    std::string extractTableName(const std::string& statement);

    // determina las columnas a seleccionar y llena selectedCols y retorna la cantidad
    int resolveSelectedColumns(const std::string& statement, const Table& table, std::string selectedCols[]);
    
    // detecta si es SELECT * o columnas especificas
    bool isSelectAll(const std::string& statement);

    // extrae las columnas especificas entre SELECT y FROM
    int parseSelectColumns(const std::string& statement, std::string selectedCols[]);

    //METODOS RELACIONADOS AL WHERE

    // retorna true si hay WHERE, false si no hay, y lo extrae
    bool parseWhere(const std::string& statement, std::string& whereColumn, std::string& whereOperator, std::string& whereValue);

    // verifica si una fila cumple la condicion WHERE
    bool rowMatchesWhere(const char* buffer, const Table& table, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue);

    //METODOS RELACIONADOS AL ORDER BY

    // retorna true si hay ORDER BY, false si no hay, y lo extrae
    bool parseOrderBy(const std::string& statement, std::string& orderColumn, bool& ascending);

    // aplica el ORDER BY al resultado si existe en el statement
    void applyOrderBy(QueryResult& result, const Table& table, const std::string& statement, const std::string selectedCols[], int selectedCount);

    // busca el indice de una columna en el arreglo de columnas seleccionadas
    int findColumnIndex(const std::string selectedCols[], int selectedCount, const std::string& colName);

    //METODOS RELACIONADOS A LA LECTURA Y DESERIALIZACION

    // convierte los bytes de una columna a string legible
    std::string deserializeValue(const char* buffer, const Column& col);

    // lee todas las filas activas del archivo binario y llena el resultado
    void readRows(const Table& table, const std::string selectedCols[], int selectedCount, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue, QueryResult& result);
    // notaaa este metodo recibe 7 argumentos, si ves que es mucho lo cambiamos hay otros con 6 tmb
};