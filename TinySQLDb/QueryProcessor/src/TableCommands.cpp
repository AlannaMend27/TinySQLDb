#define _CRT_SECURE_NO_WARNINGS
#include "TableCommands.h"
#include "Column.h"
#include <sstream>
#include <algorithm>

TableCommands::TableCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    : Commands(dataManager, catalog)
{
    //
}

// Valida que haya una base de datos activa, que exista y valida la table a crear con el systemCatalog
bool TableCommands::checkCreateTableOnCatalog(const std::string& database, QueryResult& result, Table& table)
{
    //Si no hay base de datos activa
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return false;
    }

    //Valida si la base de datos existe
    if (!this->systemCatalog.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return false;
    }

    // registrar la tabla y sus columnas en el system catalog
    bool registered = this->systemCatalog.registerTable(table);

    // si no se pudo registrar
    if (!registered)
    {
        result.success = false;
        result.message = "Error: la tabla ya existe. Use otro nombre";
        return false;
    }
    return true;
}

// Extrae el nombre de la tabla
std::string TableCommands::extractTableName(const std::string& statement)
{
    // extraer las palabras para saltarse CREATE y TABLE y llegar al nombre
    std::istringstream stream(statement);
    std::string command;
    std::string instruction;
    std::string name;
    // esto es como cin >> pero para strings
    stream >> command >> instruction >> name;

    // Para manejar el caso CREATE TABLE AS o sin el AS
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "AS")
    {
        stream >> name;
    }

    // si el nombre viene pegado al parentesis (ej: "hola(id"), cortar ahi
    // esto pasa cuando el usuario no deja espacio antes de los parentesis
    int parenPos = (int)name.find('(');
    if (parenPos != -1)
    {
        name = name.substr(0, parenPos);
    }

    return name;
}

// Extrae el contenido entre parentesis
std::string TableCommands::extractBody(const std::string& statement)
{
    //Buscar la posicion del parentesis que abre y cierra
    int open = (int)statement.find('(');
    int close = (int)statement.rfind(')');

    //Si no se encontraron o estan en orden incorrecto
    if (open == -1 || close == -1 || close <= open)
        return "";

    //Extra lo que esta dentro de los parentesis
    return statement.substr(open + 1, close - open - 1);
}

// Separa el cuerpo en definiciones de columna por coma
int TableCommands::splitColumns(const std::string& body, std::string colDefs[])
{
    int colCount = 0;
    int start = 0;

    //Buscar la primera coma
    int comma = (int)body.find(',');


    //Mientras haya comas y no se supere el maximo de columnas
    while (comma != -1 && colCount < MAX_COLUMNS)
    {
        //guardar la definicion de columna entre el inicio y la coma
        colDefs[colCount] = body.substr(start, comma - start);
        colCount++;

        // mover el inicio al siguiente caracter despues de la coma
        start = comma + 1;
        comma = (int)body.find(',', start);
    }

    // agregar lo que queda despues de la ultima coma
    colDefs[colCount] = body.substr(start);
    colCount++;

    return colCount;
}

// dado un string de tipo, resuelve el ColumnType y su tamanio en bytes
bool TableCommands::resolveType(const std::string& upperType, ColumnType& type, int& size, QueryResult& result)
{
    //Determinar el tipo de dato y su tamanio en bytes
    if (upperType == "INTEGER")
    {
        type = TYPE_INTEGER;
        size = Column::defaultSizeForType(TYPE_INTEGER);
    }
    else if (upperType == "DOUBLE")
    {
        type = TYPE_DOUBLE;
        size = Column::defaultSizeForType(TYPE_DOUBLE);
    }
    else if (upperType == "DATETIME")
    {
        type = TYPE_DATETIME;
        size = Column::defaultSizeForType(TYPE_DATETIME);
    }
    //Si es el varchar es el mas complejo
    else if (upperType.substr(0, 7) == "VARCHAR")
    {
        type = TYPE_VARCHAR;

        // el tamanio viene entre parentesis VARCHAR(n)
        int parenOpen = (int)upperType.find('(');
        int parenClose = (int)upperType.find(')');

        if (parenOpen == -1 || parenClose == -1)
        {
            result.success = false;
            result.message = "Error: VARCHAR debe especificar el tamanio. Use VARCHAR(n)";
            return false;
        }

        // extraer el numero entre los parentesis y convertirlo
        std::string sizeStr = upperType.substr(parenOpen + 1, parenClose - parenOpen - 1);
        size = std::stoi(sizeStr);
    }
    else
    {
        result.success = false;
        result.message = "Error: tipo de dato desconocido '" + upperType + "'";
        return false;
    }

    return true;
}

// De una definicion de columna llena el objeto Column
bool TableCommands::parseColumn(const std::string& colDef, const std::string& tableName, int position, int offset, Column& column, QueryResult& result)
{
    // leer el nombre y el tipo de la definicion de la columna
    std::istringstream colStream(colDef);
    std::string colName;
    std::string colType;
    colStream >> colName >> colType;

    //Verificar que se hayan leido ambos
    if (colName.empty() || colType.empty())
    {
        result.success = false;
        result.message = "Error en la definicion de la columna " + std::to_string(position + 1);
        return false;
    }

    //convertir todo a mayusculas
    std::string upperType = colType;
    std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);

    // determinar el tipo de dato y su tamanio en bytes
    ColumnType type;
    int size = 0;

    if (!resolveType(upperType, type, size, result))
        return false;

    // leer los tokens opcionales: NOT NULL, NULL, PRIMARY KEY, UNIQUE
    std::string token;
    bool nullable = true;
    ColumnConstraint constraint = CONSTRAINT_NONE;

    while (colStream >> token)
    {
        // transformar en mayusculas
        std::transform(token.begin(), token.end(), token.begin(), ::toupper);

        //Si es NOT
        if (token == "NOT")
        {
            // lo siguiente es NULL
            std::string next;
            colStream >> next;
            std::transform(next.begin(), next.end(), next.begin(), ::toupper);
            if (next == "NULL")
            {
                nullable = false;
            }
        }

        else if (token == "NULL")
        {
            nullable = true;
        }

        //Si es primary
        else if (token == "PRIMARY")
        {
            // lo siguiente es KEY
            std::string next;
            colStream >> next;
            std::transform(next.begin(), next.end(), next.begin(), ::toupper);

            if (next == "KEY")
            {
                constraint = CONSTRAINT_PRIMARY_KEY;
            }
        }
        else if (token == "UNIQUE")
        {
            constraint = CONSTRAINT_UNIQUE;
        }
    }

    // construir la columna con todos los datos extraidos
    column = Column(colName, tableName, type, size, offset, position, nullable, constraint);
    return true;
}

// Ejecuta CREATE TABLE
void TableCommands::executeCreateTable(QueryResult& result, const std::string& statement, const std::string& database)
{

    //Extrae el nombre de la tabla
    std::string tableName = extractTableName(statement);
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: CREATE TABLE <nombre> (<columnas>)";
        return;
    }

    //Extrae lo que esta entre parentesis
    std::string body = extractBody(statement);
    if (body.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Faltan parentesis en la definicion de columnas";
        return;
    }

    //separa en definiciones individuales de columna
    std::string colDefs[MAX_COLUMNS];
    int colCount = splitColumns(body, colDefs);

    //extrae cada definicion y consturye el arreglo de columnas
    Column columns[MAX_COLUMNS];
    // el offset empieza en 1 porque el byte 0 es el flag de validez del registro
    int offset = 1; 

    for (int i = 0; i < colCount; i++)
    {
        if (!parseColumn(colDefs[i], tableName, i, offset, columns[i], result))
            return;
        // avanzar segun el tamanio de la columna
        offset += columns[i].size;
    }

    //Construir la tabla
    Table table(tableName, database, columns, colCount);

    //Verificar que la tabla por crear sea valida en system catalog
    if (!checkCreateTableOnCatalog(database, result, table)) {
        return;
    }    

    // crear la tabla en la base de datos
    this->dataManager.createTable(table);

    // devolver mensajes de exito 
    result.success = true;
    result.message = "Tabla '" + tableName + "' creada exitosamente";
    return;
}