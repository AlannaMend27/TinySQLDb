#define _CRT_SECURE_NO_WARNINGS
#include "UpdateCommands.h"
#include <sstream>
#include <algorithm>

// Constructor
UpdateCommands::UpdateCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    : dataManager(dataManager), systemCatalog(catalog)
{
    //
}

// Ejecuta UPDATE <tabla> SET <columna> = <valor> [WHERE ...]
void UpdateCommands::executeUpdate(QueryResult& result, const std::string& statement, const std::string& database)
{
    // extraer el nombre de la tabla
    std::string tableName = this->extractTableName(statement);

    // si el nombre esta vacio, la sintaxis fue incorrecta
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: UPDATE <tabla> SET <columna> = <valor>";
        return;
    }

    // validar que la base de datos y la tabla existan
    if (!this->validateDBTable(database, tableName, result))
    {
        return;
    }

    // obtener la tabla desde el system catalog
    Table table = this->systemCatalog.getTable(database, tableName);

    // parsear el SET
    std::string setColumn;
    std::string setValue;
    if (!this->parseSet(statement, setColumn, setValue, result))
    {
        return;
    }

    // verificar que la columna del SET exista en la tabla
    const Column* col = table.getColumn(setColumn);

    // si la columna no existe, error de sintaxis
    if (col == nullptr)
    {
        result.success = false;
        result.message = "Error: la columna '" + setColumn + "' no existe en la tabla '" + setValue + "'.";
        return;
    }

    // verificar que el nuevo valor sea compatible con el tipo de la columna
    if (!col->isValueCompatible(setValue))
    {
        result.success = false;
        result.message = "Error: el valor '" + setValue + "' no es compatible con la columna '" + setColumn + "' de tipo " + col->typeToString();
        return;
    }

    // parsear el WHERE si existe
    std::string whereColumn;
    std::string whereOperator;
    std::string whereValue;
    bool hasWhere = this->parseWhere(statement, whereColumn, whereOperator, whereValue);

    // si hay WHERE, verificar que la columna exista
    if (hasWhere && table.getColumn(whereColumn) == nullptr)
    {
        result.success = false;
        result.message = "Error: la columna '" + whereColumn + "' del WHERE no existe en la tabla";
        return;
    }

    // actualizar las filas que coinciden con las condiciones del statement, usando el data manager
    int updatedCount = this->updateMatchingRows(table, *col, setValue, whereColumn, whereOperator, whereValue, hasWhere);

    // actualizar mensaje de exito 
    result.success = true;
    result.message = std::to_string(updatedCount) + " fila(s) actualizada(s)";
}

// extrae el nombre de la tabla donde se desea hacer el update
std::string UpdateCommands::extractTableName(const std::string& statement)
{
    std::istringstream stream(statement);
    std::string update;
    std::string name;

    // saltamos UPDATE y leemos el nombre de la tabla
    stream >> update >> name;
    return name;
}

// valida que la base de datos y la tabla existan
bool UpdateCommands::validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result)
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

// extrae la columna y el valor del SET
// SET Nombre = "Felipe"  setColumn="Nombre", setValue="Felipe"
bool UpdateCommands::parseSet(const std::string& statement, std::string& setColumn, std::string& setValue, QueryResult& result)
{
    // convertir a mayusculas para buscar SET
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar la posicion de SET en la statement colocada por el usuario
    int setPos = (int)upper.find(" SET ");

    // si no se encontro SET en al statement
    if (setPos == -1)
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: UPDATE <tabla> SET <columna> = <valor>";
        return false;
    }

    // extraer la parte despues de SET
    // puede terminar en WHERE o en el final del string
    int wherePos = (int)upper.find(" WHERE ");
    std::string setPart;

    // si se encontro el where y esta despues de la pos del set
    if (wherePos != -1 && wherePos > setPos)
        // obtener lo que esta entre el set y entre el where 
        setPart = statement.substr(setPos + 5, wherePos - setPos - 5);
    else
        // obtener lo que esta luego del set
        setPart = statement.substr(setPos + 5);

    // buscar el signo =
    int equalPos = (int)setPart.find('=');
    if (equalPos == -1)
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Falta el '=' en el SET";
        return false;
    }

    // extraer la columna (antes del =)
    setColumn = setPart.substr(0, equalPos);

    // quitar espacios al inicio y al final
    while (!setColumn.empty() && setColumn.front() == ' ')
    {
        setColumn.erase(setColumn.begin());
    }
    while (!setColumn.empty() && setColumn.back() == ' ') {
        setColumn.pop_back();
    }

    // extraer el valor (despues del =)
    setValue = setPart.substr(equalPos + 1);
    // quitar espacios al inicio y al final
    while (!setValue.empty() && setValue.front() == ' ') 
    {
        setValue.erase(setValue.begin());
    }
    while (!setValue.empty() && setValue.back() == ' ')
    {
        setValue.pop_back();
    }

    // si el valor viene entre comillas, quitarlas
    if (!setValue.empty() && setValue.front() == '"')
    {
        setValue.erase(setValue.begin());
        if (!setValue.empty() && setValue.back() == '"')
            setValue.pop_back();
    }

    // si quedo un valor vacio luego del parseo, la sintaxis fue incorrecta
    if (setColumn.empty() || setValue.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta en el SET";
        return false;
    }

    return true;
}

// extrae la condicion WHERE si existe
bool UpdateCommands::parseWhere(const std::string& statement, std::string& whereColumn, std::string& whereOperator, std::string& whereValue)
{
    // convertimos a mayusculas la statement
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // obtenemos el valor del where
    int wherePos = (int)upper.find(" WHERE ");

    // si no se encontro where
    if (wherePos == -1)
    {
        return false;
    }

    // obtener los caracteres luego del where
    std::string wherePart = statement.substr(wherePos + 7);

    // guardar los datos del where en las variables indicadas
    std::istringstream stream(wherePart);
    stream >> whereColumn >> whereOperator >> whereValue;

    // si el valor viene entre comillas, quitarlas
    if (!whereValue.empty() && whereValue.front() == '"')
    {
        whereValue.erase(whereValue.begin());
        if (!whereValue.empty() && whereValue.back() == '"')
            whereValue.pop_back();
    }

    return true;
}

// convierte los bytes de una columna a string legible
std::string UpdateCommands::deserializeValue(const char* buffer, const Column& col)
{
    // deseariza de acuerdo con el tipo de columna que se desea deserealizar
    switch (col.type)
    {
    case TYPE_INTEGER:
    {
        // convierte los datos int del buffer en un string
        int32_t num;
        memcpy(&num, buffer + col.offset, sizeof(int32_t));
        return std::to_string(num);

    }
    case TYPE_DOUBLE:
    {
        // convierte los datos de la columna de tipo doble del buffer en un string
        double num;
        memcpy(&num, buffer + col.offset, sizeof(double));
        return std::to_string(num);
    }
    case TYPE_VARCHAR:
    {
        // Creamos el string con el tamaño completo del buffer fijo
        std::string str(buffer + col.offset, col.size);

        // Buscamos dónde aparece el primer carácter nulo '\0'
        size_t nullPos = str.find('\0');

        // Si encontramos un nulo, recortamos el string hasta ahí para quitar el relleno basura
        if (nullPos != std::string::npos)
        {
            str = str.substr(0, nullPos);
        }

        // retornamos el string
        return str;
    }
    case TYPE_DATETIME:
    {
        // convierte el dato de tipo datetime d ela columna en un string legible
        int64_t timestamp;
        memcpy(&timestamp, buffer + col.offset, sizeof(int64_t));

        time_t t = (time_t)timestamp;
        struct tm* timeInfo = localtime(&t);

        // dar formato de string al tiempo con la funcion strftime
        char formatted[20];
        strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", timeInfo);

        // retornar string
        return std::string(formatted);
    }

    default:
        return "";

    }
}

// verifica si una fila cumple la condicion WHERE
bool UpdateCommands::rowMatchesWhere(const char* buffer, const Table& table,
    const std::string& whereColumn,
    const std::string& whereOperator,
    const std::string& whereValue)
{
    // obtener la columna de la statement
    const Column* col = table.getColumn(whereColumn);

    // si no se encontro la colmna, retornar falso
    if (col == nullptr) 
    {
        return false; 
    }

    // deserealizar el valor de la columna
    std::string cellValue = this->deserializeValue(buffer, *col);

    // pasar a mayusculas el operador toupper
    std::string op = whereOperator;
    std::transform(op.begin(), op.end(), op.begin(), ::toupper);

    if (op == "=")
    {
        // calmbiar el valor de esa celda
        return cellValue == whereValue;
    }
    else if (op == ">")
    {
        // en caso de que sean tipos de datos distintos, convertir antes de comparar
        if (col->type == TYPE_INTEGER || col->type == TYPE_DOUBLE) 
        {
            return std::stod(cellValue) > std::stod(whereValue);
        }
        return cellValue > whereValue;
    }
    else if (op == "<")
    {
        // en caso de que sean tipos de datos distintos, convertir antes de comparar
        if (col->type == TYPE_INTEGER || col->type == TYPE_DOUBLE)
        {
            return std::stod(cellValue) < std::stod(whereValue);
        }
        return cellValue < whereValue;
    }
    else if (op == "LIKE")
    {
        // verificar si el valor de la celda es igual o contiene el patron dado
        std::string pattern = whereValue;
        while (!pattern.empty() && pattern.front() == '*')
        {
            pattern.erase(pattern.begin());
        }
        while (!pattern.empty() && pattern.back() == '*')
        {
            pattern.pop_back();
        } 

        // Convertir el patrón limpio a mayúsculas
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::toupper);

        //Convertir también el valor de la celda a mayúsculas (en una variable temporal)
        std::string cellValueUpper = cellValue;
        std::transform(cellValueUpper.begin(), cellValueUpper.end(), cellValueUpper.begin(), ::toupper);

        // retornar si encontro el valor 
        return cellValueUpper.find(pattern) != std::string::npos;
    }
    else if (op == "NOT")
    {
        // retornar si son diferentes
        return cellValue != whereValue;
    }

    return false;
}

// serializa un solo valor dentro del buffer en la posicion de su columna
void UpdateCommands::serializeSingleValue(char* buffer, const Column& col, const std::string& value)
{
    // verifica cual es el tipo de dato que almacena la columna
    switch (col.type)
    {
    case TYPE_INTEGER:
    {
        // convierte a interger 
        int32_t num = std::stoi(value);
        // copiar en el buffer
        memcpy(buffer + col.offset, &num, sizeof(int32_t));
        break;
    }
    case TYPE_DOUBLE:
    {
        // convierte a tipo de dato double
        double num = std::stod(value);
        // copiar en el buffer 
        memcpy(buffer + col.offset, &num, sizeof(double));
        break;
    }
    case TYPE_VARCHAR:
    {
        // copia en el buffer haciendo una copia segura con strncpy
        memset(buffer + col.offset, 0, col.size);
        strncpy(buffer + col.offset, value.c_str(), col.size);
        break;
    }
    case TYPE_DATETIME:
    {
        // parsear el string "YYYY-MM-DD HH:MM:SS" a timestamp unix

        // usamos struct tm para guardar cada parte de la fecha
        struct tm timeInfo = {};

        // sscanf lee los campos del string con formato de fecha
        sscanf(value.c_str(), "%d-%d-%d %d:%d:%d",
            &timeInfo.tm_year,
            &timeInfo.tm_mon,
            &timeInfo.tm_mday,
            &timeInfo.tm_hour,
            &timeInfo.tm_min,
            &timeInfo.tm_sec);

        // tm_year se cuenta desde 1900 y tm_mon desde 0
        timeInfo.tm_year -= 1900;
        timeInfo.tm_mon -= 1;

        // mktime convierte el struct tm a unix timestamp (la cant de segundos transcurridos desde 1970)
        int64_t timestamp = (int64_t)mktime(&timeInfo);

        // guarda ese numero en el buffer
        memcpy(buffer + col.offset, &timestamp, sizeof(int64_t));
        break;

    }
    default:
        break;
    }
}

// recorre todas las filas, modifica las que cumplan el WHERE y las reescribe
int UpdateCommands::updateMatchingRows(const Table& table, const Column& setCol, const std::string& setValue,
                                       const std::string& whereColumn, const std::string& whereOperator,
                                       const std::string& whereValue, bool hasWhere)
{
    // pedir al dataManager todas las filas en un solo buffer
    int rowCount = 0;
    char* allRows = this->dataManager.readAllRows(table, rowCount);

    // si no hay filas, retornar 0
    if (allRows == nullptr)
    {
        return 0;
    }

    int updatedCount = 0;

    // recorrer cada fila dentro del buffer grande
    for (int i = 0; i < rowCount; i++)
    {
        // Nos desplzamos en el buffer allRows hasta donde empieza la fila de cada iteracion
        // Esta linea es importante pq le pasamos a los demas metodos el mismo buffer que tiene todas las filas
        // pero con el puntero apuntando al lugar donde incia la fila que se debe cambiar en caso de que haya match
        char* row = allRows + (i * table.rowSize);

        // saltar filas eliminadas
        if (row[0] == 0) {
            continue;
        }

        // verificar si la fila tiene where y si cumple el where
        if (!hasWhere || this->rowMatchesWhere(row, table, whereColumn, whereOperator, whereValue))
        {
            // modificar el valor en el buffer de esta fila 
            this->serializeSingleValue(row, setCol, setValue);

            // pedir al dataManager que reescriba esta fila en su posicion
            this->dataManager.writeRowAt(table, i, row);

            updatedCount++;
        }
    }

    delete[] allRows;
    return updatedCount;
}
