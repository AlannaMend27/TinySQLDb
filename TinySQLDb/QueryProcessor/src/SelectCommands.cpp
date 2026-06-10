#define _CRT_SECURE_NO_WARNINGS
#include "SelectCommands.h"
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstring>

// Constructor
SelectCommands::SelectCommands(StoredDataManager& dataManager, SystemCatalog& catalog) : dataManager(dataManager), systemCatalog(catalog)
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

    // leer las filas aplicando el filtro WHERE si existe
    this->readRows(table, selectedCols, selectedCount, whereColumn, whereOperator, whereValue, result);

    // aplicar ORDER BY si existe
    this->applyOrderBy(result, table, statement, selectedCols, selectedCount);

    result.success = true;
    result.message = std::to_string(result.rowCount) + " fila(s) encontrada(s)";
    return;
}

//METODOS PARA VALIDACIOES

// Valida que la base de datos y la tabla existan
bool SelectCommands::validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result)
{
    // verificar que haya una base de datos activa
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return false;
    }

    // verificar que la base de datos exista
    if (!this->systemCatalog.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return false;
    }

    // verificar que la tabla exista
    if (!this->systemCatalog.tableExists(database, tableName))
    {
        result.success = false;
        result.message = "Error: la tabla '" + tableName + "' no existe";
        return false;
    }

    return true;
}

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

//METODOS RELACIONADOS AL WHERE
// 
//extrae la condicion WHERE del statement y tambien retorna true si hay WHERE, false si no hay
bool SelectCommands::parseWhere(const std::string& statement, std::string& whereColumn, std::string& whereOperator, std::string& whereValue)
{
    // convertir a mayusculas para buscar WHERE
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion del WHERE
    int wherePos = (int)upper.find(" WHERE ");
    if (wherePos == -1)
    {
        // no hay WHERE
        return false;
    }

    // extraer la parte despues del WHERE
    std::string wherePart = statement.substr(wherePos + 7);

    // leer columna, operador y valor
    std::istringstream stream(wherePart);
    // leer la columna, el operador y el valor del WHERE
    stream >> whereColumn >> whereOperator >> whereValue;

    return true;
}

// verifica si una fila cumple la condicion WHERE
bool SelectCommands::rowMatchesWhere(const char* buffer, const Table& table, const std::string& whereColumn, const std::string& whereOperator, const std::string& whereValue)
{
    // obtener la columna del WHERE
    const Column* col = table.getColumn(whereColumn);
    if (col == nullptr)
    {
        return false;
    }

    // deserializar el valor de esa columna a string para comparar
    std::string cellValue = this->deserializeValue(buffer, *col);

    // convertir operador a mayusculas para comparar
    std::string op = whereOperator;
    std::transform(op.begin(), op.end(), op.begin(), ::toupper);

    //Revisamos que operador corresponde, retornamos la condicion segun cada uno
    if (op == "=") {
        return cellValue == whereValue;
    }
    else if (op == ">") {
        // para numeros comparamos como numeros, para strings como strings
        if (col->type == TYPE_INTEGER || col->type == TYPE_DOUBLE) {
            return std::stod(cellValue) > std::stod(whereValue);
        }
        return cellValue > whereValue;
    }
    else if (op == "<") {
        //Lo mismo pero al reves
        if (col->type == TYPE_INTEGER || col->type == TYPE_DOUBLE) {
            return std::stod(cellValue) < std::stod(whereValue);
        }
        return cellValue < whereValue;
    }
    else if (op == "LIKE") {
        // LIKE "texto" verifica si el valor contiene el patron{
        std::string pattern = whereValue;

        // quitar asteriscos al inicio y al final
        while (!pattern.empty() && pattern.front() == '*') {
            pattern.erase(pattern.begin());
        }
        while (!pattern.empty() && pattern.back() == '*') {
            pattern.pop_back();
        }

        // verificar si el valor contiene el patron
        return cellValue.find(pattern) != std::string::npos;

    }
    else if (op == "NOT")
    {
        // NOT verifica que el valor sea diferente
        return cellValue != whereValue;
    }

    // operador no reconocido
    return false;
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

//METODOS RELACIONADOS A LA LECTURA Y DESERIALIZACION

// Convierte los bytes de una columna a string legible
// buffer, es la fila comleta desde el disco como bytes
// col, es la columna que queremos leer 
std::string SelectCommands::deserializeValue(const char* buffer, const Column& col)
{
    if (col.type == TYPE_INTEGER)
    {
        //Copiar 4 bytes del buffer a num y interpretarlos como entero
        int32_t num;
        memcpy(&num, buffer + col.offset, sizeof(int32_t));
        return std::to_string(num);
    }
    else if (col.type == TYPE_DOUBLE)
    {
        // copiar 8 bytes del buffer al num y interpretarlos como double
        double num;
        memcpy(&num, buffer + col.offset, sizeof(double));
        return std::to_string(num);
    }
    else if (col.type == TYPE_VARCHAR)
    {
        // leer el varchar y quitar los caracteres nulos del final
        std::string value(buffer + col.offset, col.size);
        // el varchar se guarda con \0 al final para rellenar el tamanio fijo
        // hay que cortarlo en el primer \0 para obtener solo el texto real
        int end = (int)value.find('\0');
        if (end != -1)
        {
            value = value.substr(0, end);
        }
        return value;
    }
    else if (col.type == TYPE_DATETIME)
    {
        // leer el unix timestamp guardado como 8 bytes
        int64_t timestamp;
        memcpy(&timestamp, buffer + col.offset, sizeof(int64_t));

        // convertir timestamp unix a string legible YYYY-MM-DD HH:MM:SS
        time_t t = (time_t)timestamp;
        struct tm* timeInfo = localtime(&t);
        char formatted[20];
        strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", timeInfo);
        return std::string(formatted);
    }
       
    return "";
}

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



