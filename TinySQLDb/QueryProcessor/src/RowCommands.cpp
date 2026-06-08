#define _CRT_SECURE_NO_WARNINGS
#include <sstream>
#include <algorithm>
#include "RowCommands.h"

RowCommands::RowCommands()
{
    //
}

// Extrae el nombre de la tabla del statement
std::string RowCommands::extractTableNameForRow(const std::string& statement)
{
    // convertimos el string para poder leerlo palara por palabra
    std::istringstream stream(statement);

    // crear variables para guardar los datos que leemos
    std::string insert;
    std::string into;
    std::string name;

    // saltamos INSERT e INTO y leemos el nombre de la tabla
    stream >> insert >> into >> name;
    return name;
}

// Extrae el contenido entre los parentesis de VALUES(...)
std::string RowCommands::extractValuesBody(const std::string& statement)
{
    // convertir a mayusculas solo para buscar la palabra VALUES
    std::string upper = statement;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // buscar donde empieza VALUES
    int valuesPos = (int)upper.find("VALUES");
    if (valuesPos == -1) {
        return "";
    }

    // buscar el parentesis que abre despues de VALUES
    int open = (int)statement.find('(', valuesPos);

    // buscar el parentesis que cierra
    int close = (int)statement.rfind(')');

    // en caso de que no se haya encontrado el paratensis de inicio o de cierre, retornar string vacio
    if (open == -1 || close == -1 || close <= open) {
        return "";
    }

    // devolver el string una posicion mayor al inicio(justo despues de "(") y una posicion menor al final (justo antes de ")")
    return statement.substr(open + 1, close - open - 1);
}

// Separa los valores por coma respetando strings entre comillas
// "1, \"Isaac\", \"Ramirez\"" → ["1", "Isaac", "Ramirez"]
int RowCommands::splitValues(const std::string& body, std::string values[])
{
    // variables de contabilizacion para llevar el control
    int count = 0;
    int i = 0;

    // obtener la cantidad de caracteres(contandos numeros y espacios en blanco)
    int len = (int)body.size();

    // recorrer el string body mientras haya caracteres en el o se haya llegado al max de columnas
    while (i < len && count < MAX_COLUMNS)
    {
        // saltar espacios
        while (i < len && body[i] == ' ') {
            i++;
        }

        std::string current = "";

        // si se cuentra una comilla
        if (body[i] == '"')
        {
            // valor entre comillas — leer hasta la comilla de cierre
            i++; // saltar la comilla de apertura
            while (i < len && body[i] != '"')
            {
                current += body[i];
                i++;
            }
            i++; // saltar la comilla de cierre
        }
        else
        {
            // valor sin comillas — leer hasta la coma o fin
            while (i < len && body[i] != ',')
            {
                current += body[i];
                i++;
            }
            // quitar espacios al final del valor
            while (!current.empty() && current.back() == ' ')
                current.pop_back();
        }

        // guardar el valor en el string 
        values[count] = current;

        // actualizar cant parametros
        count++;

        // saltar la coma
        if (i < len && body[i] == ',') i++;
    }

    return count;
}

// Ejecuta INSERT INTO <tabla> VALUES(...)
QueryResult RowCommands::executeInsert(const std::string& statement, const std::string& database, StoredDataManager& dataManager)
{
    QueryResult result;

    // verificar que haya una base de datos activa
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return result;
    }

    // verificar que la base de datos exista
    if (!dataManager.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return result;
    }

    // extraer el nombre de la tabla
    std::string tableName = this->extractTableNameForRow(statement);
    if (tableName.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Use: INSERT INTO <tabla> VALUES(...)";
        return result;
    }

    // verificar que la tabla exista
    if (!dataManager.tableExists(database, tableName))
    {
        result.success = false;
        result.message = "Error: la tabla '" + tableName + "' no existe";
        return result;
    }

    // extraer el cuerpo de VALUES(...)
    std::string body = this->extractValuesBody(statement);
    if (body.empty())
    {
        result.success = false;
        result.message = "Sintaxis incorrecta. Faltan los valores a insertar";
        return result;
    }

    // separar los valores guardandolos en un array y obteniendo la cantidad de valores obtenidos
    std::string values[MAX_COLUMNS];
    int valueCount = this->splitValues(body, values);

    // pedir al dataManager que inserte la fila
    // el dataManager se encarga de validar tipos y escribir en disco
    bool inserted = dataManager.insertRow(database, tableName, values, valueCount);

    if (!inserted)  
    {
        result.success = false;
        result.message = "Error al insertar en la tabla '" + tableName + "'";
        return result;
    }

    result.success = true;
    result.message = "1 fila insertada en '" + tableName + "'";
    return result;
}