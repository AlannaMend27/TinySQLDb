#pragma once
#include <string>
#include "QueryResult.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include "Column.h"
#include "SortAlgorithms.h"
#include "Commands.h"
#include "indexManager.h"

// SelectCommands : maneja el comando SELECT y contiene los metodos para ello
class SelectCommands : public Commands {
public:

    // Constructor
    SelectCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager);


    // ejecuta SELECT * FROM <tabla>
    void executeSelect(QueryResult& result, const std::string& statement, const std::string& database);

private:

    //Administrador de index
    IndexManager& indexManager;

    // Atributos privados
    SortAlgorithms quickSorter;

    //METODOS PARA VALIDACIONES

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

    //METODOS RELACIONADOS AL ORDER BY

    // retorna true si hay ORDER BY, false si no hay, y lo extrae
    bool parseOrderBy(const std::string& statement, std::string& orderColumn, bool& ascending);

    // aplica el ORDER BY al resultado si existe en el statement
    void applyOrderBy(QueryResult& result, const Table& table, const std::string& statement, const std::string selectedCols[], int selectedCount);

    // busca el indice de una columna en el arreglo de columnas seleccionadas
    int findColumnIndex(const std::string selectedCols[], int selectedCount, const std::string& colName);

    //METODOS RELACIONADOS A LA LECTURA 

    // lee todas las filas activas del archivo binario y llena el resultado
    void readRows(const Table& table, const std::string selectedCols[], int selectedCount, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue, QueryResult& result);

    //METODOS RELACIONADOS CON INDEX

    // lee una fila especifica del disco usando el indice en lugar de busqueda secuencial
    void readRowByIndex(const Table& table, long position, const std::string selectedCols[], int selectedCount, QueryResult& result);

    //ejecuta la lectura usando indice o busqueda secuencial segun corresponda
    void executeRead(QueryResult & result, const Table & table, const std::string selectedCols[], int selectedCount, const std::string & whereColumn, const std::string & whereOperator, const std::string & whereValue);

    //METODOS RELACIONADOS AL SYSTEM CATALOG

    // verifica si el nombre corresponde a una de las 4 tablas reservadas del system catalog
    bool isSystemCatalogTable(const std::string& tableName);

    // ejecuta el SELECT sobre una de las tablas reservadas del system catalog
    void executeSelectSystemCatalog(QueryResult& result, const std::string& tableName);

    // llena el resultado con el listado de bases de datos
    void selectSystemDatabases(QueryResult& result);

    // llena el resultado con el listado de tablas
    void selectSystemTables(QueryResult& result);

    // llena el resultado con el listado de columnas
    void selectSystemColumns(QueryResult& result);

    // llena el resultado con el listado de indices
    void selectSystemIndexes(QueryResult& result);
};