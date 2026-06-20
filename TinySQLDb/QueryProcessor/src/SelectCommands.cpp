#define _CRT_SECURE_NO_WARNINGS
#include "SelectCommands.h"
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstring>

// Constructor
SelectCommands::SelectCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager)
    : Commands(dataManager, catalog), indexManager(indexManager)
{
    //
}

// Metodo que ejecuta SELECT, coordina el parseo, validacion, lectura y ordenamiento
void SelectCommands::executeSelect(QueryResult& result, const std::string& statement, const std::string& database)
{
    // extraer el nombre de la tabla despues del FROM
    std::string tableName = this->extractTableName(statement);

    // si en el nombre de la tabla no hay nada
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: SELECT * FROM <tabla>";
        return;
    }

    // si la tabla solicitada es una de las 4 reservadas del system catalog,
    if (this->isSystemCatalogTable(tableName))
    {
        this->executeSelectSystemCatalog(result, tableName);
        return;
    }

    // validar que la base de datos y la tabla existan
    if (!this->validateDBTable(database, tableName, result))
    {
        return;
    }

    // obtener la estructura de la tabla del catalog
    Table table = this->systemCatalog.getTable(database, tableName);

    // determinar las columnas a seleccionar (todas o especificas)
    std::string selectedCols[MAX_COLUMNS];
    int selectedCount = this->resolveSelectedColumns(statement, table, selectedCols);

    // verificar que las columnas seleccionadas existan en la tabla
    if (!this->validateColumns(table, selectedCols, selectedCount, result))
    {
        return;
    }

    // llenar los nombres de columnas en el resultado
    result.columnCount = selectedCount;
    for (int i = 0; i < selectedCount; i++)
    {
        result.columnNames[i] = selectedCols[i];
    }

    // parsear WHERE si existe
    std::string whereColumn;
    std::string whereOperator;
    std::string whereValue;
    this->parseWhere(statement, whereColumn, whereOperator, whereValue);

    // ejecutar la lectura usando indice o busqueda secuencial segun corresponda
    this->executeRead(result, table, selectedCols, selectedCount, whereColumn, whereOperator, whereValue);

    // aplicar ORDER BY si existe
    this->applyOrderBy(result, table, statement, selectedCols, selectedCount);

    result.success = true;
    result.message = std::to_string(result.rowCount) + " fila(s) encontrada(s)";
    return;
}

//METODOS PARA VALIDACIOES

// verifica que las columnas seleccionadas existan en la tabla
bool SelectCommands::validateColumns(const Table& table, const std::string selectedCols[], int selectedCount, QueryResult& result)
{
    for (int i = 0; i < selectedCount; i++)
    {
        if (!table.hasColumn(selectedCols[i]))
        {
            result.success = false;
            result.message = "Error: la columna '" + selectedCols[i] + "' no existe en la tabla";
            return false;
        }
    }
    return true;
}

//METODOS DE PARSEO GENERAL

// Extrae el nombre de la tabla del statement despues del FROM
std::string SelectCommands::extractTableName(const std::string& statement)
{
    // convertir a mayusculas para buscar FROM sin importar el caso (esto es opcional)
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion del FROM
    int fromPos = (int)upper.find(" FROM ");
    if (fromPos == -1)
    {
        return "";
    }

    // leer la palabra despues del FROM del statement original (para preservar el caso)
    std::istringstream stream(statement.substr(fromPos + 6));
    std::string tableName;
    stream >> tableName;

    return tableName;
}

// determina las columnas a seleccionar llena selectedCols y retorna la cantidad
int SelectCommands::resolveSelectedColumns(const std::string& statement, const Table& table, std::string selectedCols[])
{
    // si es SELECT * tomamos todas las columnas de la tabla en orden
    if (this->isSelectAll(statement))
    {
        for (int i = 0; i < (int)table.columnCount; i++)
        {
            selectedCols[i] = table.columns[i].name;
        }
        return table.columnCount;
    }

    // si no, extraer las columnas especificas entre SELECT y FROM
    return this->parseSelectColumns(statement, selectedCols);
}

// detecta si es SELECT * o columnas especificas
bool SelectCommands::isSelectAll(const std::string& statement)
{
    // convertir a mayusculas para comparar
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    //  si el segundo argumento es * es SELECT *
    std::istringstream stream(upper);
    std::string select;
    std::string next;
    stream >> select >> next;

    return next == "*";
}

// extrae las columnas especificas entre SELECT y FROM (por que estan en medio de ellas)
int SelectCommands::parseSelectColumns(const std::string& statement, std::string selectedCols[])
{
    // convertir a mayusculas para buscar FROM
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // encontrar donde empieza el FROM
    int fromPos = (int)upper.find(" FROM ");
    if (fromPos == -1)
    {
        return 0;
    }

    // extraer la parte entre SELECT y FROM
    std::string colsPart = statement.substr(7, fromPos - 7);

    // separar por coma
    int count = 0;
    int start = 0;
    int comma = (int)colsPart.find(',');

    while (comma != -1 && count < (int)MAX_COLUMNS)
    {
        std::string col = colsPart.substr(start, comma - start);

        // quitar espacios al inicio y al final
        while (!col.empty() && col.front() == ' ')
        {
            col.erase(col.begin());
        }
        while (!col.empty() && col.back() == ' ')
        {
            col.pop_back();
        }

        selectedCols[count] = col;
        count++;
        start = comma + 1;
        comma = (int)colsPart.find(',', start);
    }

    // ultima columna
    std::string last = colsPart.substr(start);
    while (!last.empty() && last.front() == ' ')
    {
        last.erase(last.begin());
    }
    while (!last.empty() && last.back() == ' ')
    {
        last.pop_back();
    }

    if (!last.empty())
    {
        selectedCols[count] = last;
        count++;
    }

    return count;
}

//METODOS RELACIONADOS AL ORDER BY

// extrae el ORDER BY del statement
// retorna true si hay ORDER BY, false si no hay
bool SelectCommands::parseOrderBy(const std::string& statement, std::string& orderColumn, bool& ascending)
{
    // convertir a mayusculas para buscar ORDER BY
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion del ORDER BY
    int orderPos = (int)upper.find(" ORDER BY ");
    if (orderPos == -1)
    {
        // no hay ORDER BY
        return false;
    }

    // extraer la parte despues del ORDER BY
    std::string orderPart = statement.substr(orderPos + 10);

    // leer la columna y la direccion
    std::istringstream stream(orderPart);
    std::string direction;
    stream >> orderColumn >> direction;

    // convertir direccion a mayusculas para comparar
    std::transform(direction.begin(), direction.end(), direction.begin(), ::toupper);

    // si no dice DESC, es ASC por defecto
    ascending = (direction != "DESC");

    return true;
}

// aplica el ORDER BY al resultado si existe en el statement
void SelectCommands::applyOrderBy(QueryResult& result, const Table& table, const std::string& statement, const std::string selectedCols[], int selectedCount)
{
    // extraer la columna y direccion del ORDER BY
    std::string orderColumn;
    bool ascending = true;
    bool hasOrderBy = this->parseOrderBy(statement, orderColumn, ascending);

    // si no hay ORDER BY o hay menos de dos filas, no hay nada que ordenar
    if (!hasOrderBy || result.rowCount < 2)
    {
        return;
    }

    // buscar el indice de la columna de orden en las columnas seleccionadas
    int colIndex = this->findColumnIndex(selectedCols, selectedCount, orderColumn);
    if (colIndex == -1)
    {
        return;
    }

    // obtener el tipo de la columna para comparar correctamente
    const Column* col = table.getColumn(orderColumn);
    if (col == nullptr)
    {
        return;
    }

    // ordenar las filas usando Quicksort
    this->quickSorter.quickSort(result.rows, 0, result.rowCount - 1, colIndex, col->type, ascending);
}

// busca el indice de una columna en el arreglo de columnas seleccionadas
int SelectCommands::findColumnIndex(const std::string selectedCols[], int selectedCount, const std::string& colName)
{
    for (int i = 0; i < selectedCount; i++) {
        //Si lo encuentra
        if (selectedCols[i] == colName)
        {
            return i;
        }
    }

    // columna no encontrada
    return -1;
}

//METODOS RELACIONADOS A LA LECTURA 

// Lee todas las filas activas del archivo binario y llena el resultado
void SelectCommands::readRows(const Table& table, const std::string selectedCols[], int selectedCount, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue, QueryResult& result)
{
    // construir la ruta del archivo binario de la tabla
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";

    // abrir el archivo para lectura binaria
    std::ifstream file(tablePath, std::ios::binary);
    if (!file.is_open())
    {
        result.rowCount = 0;
        return;
    }

    // buffer para leer una fila a la vez
    char* buffer = new char[table.rowSize];
    result.rowCount = 0;

    // leer fila por fila
    while (file.read(buffer, table.rowSize))
    {

        // desencriptar la fila leida del disco
        this->dataManager.encryptBuffer(buffer, table.rowSize);

        // saltar filas eliminadas (flag = 0)
        if (buffer[0] == 0)
        {
            continue;
        }

        // si hay WHERE, verificar si la fila cumple la condicion
        if (!whereColumn.empty() && !this->rowMatchesWhere(buffer, table, whereColumn, whereOperator, whereValue))
        {
            continue;
        }

        // deserializar solo las columnas seleccionadas
        for (int i = 0; i < selectedCount; i++)
        {
            //Obtenemos las cols seleccionadas 
            const Column* col = table.getColumn(selectedCols[i]);
            if (col == nullptr)
            {
                continue;
            }
            result.rows[result.rowCount][i] = this->deserializeValue(buffer, *col);
        }

        result.rowCount++;

        // no superar el maximo de filas
        if (result.rowCount >= (int)MAX_ROWS)
        {
            break;
        }
    }

    // liberar el buffer
    delete[] buffer;
}

//METODOS RELACIONADOS A INDEX

// lee una fila especifica del disco usando la posicion del indice
void SelectCommands::readRowByIndex(const Table& table, long position, const std::string selectedCols[], int selectedCount, QueryResult& result)
{
    // construir la ruta del archivo binario
    std::string tablePath = DATA_PATH + table.dbName + "/" + table.name + ".bin";

    // abrir el archivo para lectura binaria
    std::ifstream file(tablePath, std::ios::binary);
    if (!file.is_open())
    {
        result.rowCount = 0;
        return;
    }

    // saltar directamente a la posicion indicada por el indice
    file.seekg(position, std::ios::beg);

    // buffer para leer la fila
    char* buffer = new char[table.rowSize];

    // leer la fila en esa posicion
    if (!file.read(buffer, table.rowSize))
    {
        delete[] buffer;
        result.rowCount = 0;
        return;
    }

    // desencriptar la fila leida del disco
    this->dataManager.encryptBuffer(buffer, table.rowSize);

    // verificar si la fila esta eliminada despues de desencriptar
    if (buffer[0] == 0)
    {
        delete[] buffer;
        result.rowCount = 0;
        return;
    }

    // deserializar las columnas seleccionadas
    for (int i = 0; i < selectedCount; i++)
    {
        const Column* col = table.getColumn(selectedCols[i]);
        if (col == nullptr)
        {
            continue;
        }
        result.rows[0][i] = this->deserializeValue(buffer, *col);
    }

    result.rowCount = 1;
    delete[] buffer;
}

// ejecuta la lectura usando indice o busqueda secuencial segun corresponda
void SelectCommands::executeRead(QueryResult& result, const Table& table, const std::string selectedCols[], int selectedCount, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue)
{
    // si hay WHERE con = y hay indice en esa columna, usar busqueda por indice
    if (!whereColumn.empty() && whereOperator == "=" && this->indexManager.hasIndex(table.name, whereColumn))
    {
        // obtener el indice activo
        ActiveIndex* activeIndex = this->indexManager.getIndex(table.name, whereColumn);

        // buscar la posicion en disco usando el arbol
        long position = -1;
        if (activeIndex->type == INDEX_BST && activeIndex->BST != nullptr)
        {
            // buscar en el arbol BST
            position = activeIndex->BST->search(whereValue);
        }
        else if (activeIndex->bTree != nullptr)
        {
            // buscar en el arbol BTREE
            position = activeIndex->bTree->search(whereValue);
        }

        if (position == -1)
        {
            // el valor no existe en el indice
            result.success = true;
            result.message = "0 fila(s) encontrada(s)";
            result.rowCount = 0;
            return;
        }

        // leer la fila directamente usando la posicion del indice
        this->readRowByIndex(table, position, selectedCols, selectedCount, result);
    }
    else
    {
        // no hay indice o el operador no es = — busqueda secuencial
        this->readRows(table, selectedCols, selectedCount, whereColumn, whereOperator, whereValue, result);
    }
}

//METODOS RELACIONADOS AL SYSTEM CATALOG

// verifica si el nombre corresponde a una de las 4 tablas reservadas del system catalog
bool SelectCommands::isSystemCatalogTable(const std::string& tableName)
{
    return tableName == "SystemDatabases" || tableName == "SystemTables" ||
        tableName == "SystemColumns" || tableName == "SystemIndexes";
}

// ejecuta el SELECT sobre una de las tablas reservadas del system catalog
// decide cual metodo llamar segun el nombre solicitado
void SelectCommands::executeSelectSystemCatalog(QueryResult& result, const std::string& tableName)
{
    if (tableName == "SystemDatabases")
    {
        this->selectSystemDatabases(result);
    }
    else if (tableName == "SystemTables")
    {
        this->selectSystemTables(result);
    }
    else if (tableName == "SystemColumns")
    {
        this->selectSystemColumns(result);
    }
    else if (tableName == "SystemIndexes")
    {
        this->selectSystemIndexes(result);
    }

    result.success = true;
    result.message = std::to_string(result.rowCount) + " fila(s) encontrada(s)";
}

// llena el resultado con el listado de bases de datos existentes
void SelectCommands::selectSystemDatabases(QueryResult& result)
{
    // pedir al catalog todas las bases de datos activas
    int count = 0;
    Database* databases = this->systemCatalog.getAllDatabases(count);

    // una sola columna con el nombre de cada base de datos
    result.columnCount = 1;
    result.columnNames[0] = "Nombre";

    // llenar cada fila del resultado
    for (int i = 0; i < count; i++)
    {
        result.rows[i][0] = databases[i].name;
    }

    result.rowCount = count;

    // liberar el arreglo dinamico que entrega el catalog
    delete[] databases;
}

// llena el resultado con el listado de tablas y su base de datos
void SelectCommands::selectSystemTables(QueryResult& result)
{
    // pedir al catalog todas las tablas activas de cualquier base de datos
    int count = 0;
    Table* tables = this->systemCatalog.getAllTables(count);

    // dos columnas: nombre de la tabla y base de datos a la que pertenece
    result.columnCount = 2;
    result.columnNames[0] = "Nombre";
    result.columnNames[1] = "BaseDatos";

    // llenar cada fila del resultado
    for (int i = 0; i < count; i++)
    {
        result.rows[i][0] = tables[i].name;
        result.rows[i][1] = tables[i].dbName;
    }

    result.rowCount = count;

    // liberar el arreglo dinamico que entrega el catalog
    delete[] tables;
}

// llena el resultado con el listado de columnas de todas las tablas
void SelectCommands::selectSystemColumns(QueryResult& result)
{
    // pedir al catalog todas las columnas activas de cualquier tabla
    int count = 0;
    Column* columns = this->systemCatalog.getAllColumns(count);

    // cinco columnas: tabla, columna, tipo, nullable y constraint
    result.columnCount = 5;
    result.columnNames[0] = "Tabla";
    result.columnNames[1] = "Columna";
    result.columnNames[2] = "Tipo";
    result.columnNames[3] = "Nullable";
    result.columnNames[4] = "Constraint";

    // llenar cada fila del resultado
    for (int i = 0; i < count; i++)
    {
        result.rows[i][0] = columns[i].tableName;
        result.rows[i][1] = columns[i].name;
        result.rows[i][2] = columns[i].typeToString();

        // convertir el booleano nullable a texto legible
        if (columns[i].nullable)
        {
            result.rows[i][3] = "1";
        }
        else
        {
            result.rows[i][3] = "0";
        }

        // convertir el constraint al texto legible correspondiente
        if (columns[i].constraint == CONSTRAINT_PRIMARY_KEY)
        {
            result.rows[i][4] = "PRIMARY KEY";
        }
        else if (columns[i].constraint == CONSTRAINT_UNIQUE)
        {
            result.rows[i][4] = "UNIQUE";
        }
        else
        {
            result.rows[i][4] = "NONE";
        }
    }

    result.rowCount = count;

    // liberar el arreglo dinamico que entrega el catalog
    delete[] columns;
}

// llena el resultado con el listado de indices creados
void SelectCommands::selectSystemIndexes(QueryResult& result)
{
    // pedir al catalog todos los indices activos
    int count = 0;
    Index* indexes = this->systemCatalog.getAllIndexes(count);

    // cuatro columnas: nombre del indice, tabla, columna y tipo
    result.columnCount = 4;
    result.columnNames[0] = "Nombre";
    result.columnNames[1] = "Tabla";
    result.columnNames[2] = "Columna";
    result.columnNames[3] = "Tipo";

    // llenar cada fila del resultado
    for (int i = 0; i < count; i++)
    {
        result.rows[i][0] = indexes[i].name;
        result.rows[i][1] = indexes[i].tableName;
        result.rows[i][2] = indexes[i].columnName;
        result.rows[i][3] = indexes[i].typeToString();
    }

    result.rowCount = count;

    // liberar el arreglo dinamico que entrega el catalog
    delete[] indexes;
}
