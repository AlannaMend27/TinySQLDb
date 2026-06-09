#define _CRT_SECURE_NO_WARNINGS
#include <sstream>
#include <algorithm>
#include "RowCommands.h"

RowCommands::RowCommands(StoredDataManager& dataManager, SystemCatalog& catalog)
    :dataManager(dataManager), systemCatalog(catalog)
{
    //
}

// Ejecuta INSERT INTO <tabla> VALUES(...)
QueryResult RowCommands::executeInsert(const std::string& statement, const std::string& database)
{
    QueryResult result;

    // extraer el nombre de la tabla
    std::string tableName = this->extractTableNameForRow(statement);

    // obtener tabla donde insertaremos la fila
    Table table = this->systemCatalog.getTable(database, tableName);

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

    // validad que se pueda registrar la fila en la tabla
    if (!validateAndRegisterTable(database, table, result, tableName, values, valueCount))
    {
        return result; // result con el mensaje de error
    }

    // parsear los datos de la consulta y guardarlos en un buffer
    char* buffer = this->serializeRowValues(table, values, table.rowSize);

    // insertar fila en tabla por medio de datamanager (pasandole el buffer con los datos ya parseados)
    bool tryInsert = this->dataManager.insertRow(database, tableName, buffer, table.rowSize);

    // libermaos la memoria del buffer
    delete[] buffer;

    // verificar que la insercion fue exitosa
    if (!tryInsert)
    {
        result.success = false;
        result.message = "Error al escribir en el archivo de la tabla '" + tableName + "'";
        return result;
    }

    result.success = true;
    result.message = "1 fila insertada en '" + tableName + "'";
    return result;
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

// valida con system catalog si es posible insertar la fila
bool RowCommands::validateAndRegisterTable(const std::string& database, const Table& table, QueryResult& result, const std::string tableName, const std::string values[], const int valueCount)
{
    // verifica que haya una base de datos activa
    if (database.empty())
    {
        result.success = false;
        result.message = "Error: no hay base de datos seleccionada. Use SET DATABASE primero";
        return false;
    }

    // verifica que la base de datos exista
    if (!this->systemCatalog.databaseExists(database))
    {
        result.success = false;
        result.message = "Error: la base de datos '" + database + "' no existe";
        return false;
    }

    // verifica que la tabla exista
    if (!this->systemCatalog.tableExists(database, tableName))
    {
        result.success = false;
        result.message = "Error: la tabla '" + tableName + "' no existe";
        return false;
    }

    if (!this->systemCatalog.validationsToInsertRow(table, values, valueCount))
    {
        result.success = false;
        result.message = "Error: los valores ingresados no coinciden con el tipo solicitado";
        return false;
    }

    return true;
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

char* RowCommands::serializeRowValues(const Table& table, const std::string values[], const int rowSize)
{
    // construir el buffer de bytes que representa la fila
    char* buffer = new char[rowSize];
    memset(buffer, 0, rowSize);
    buffer[0] = 1;

    // serializar cada valor en su posicion correcta dentro del buffer
    for (int i = 0; i < (int)table.columnCount; i++)
    {
        // obtener la columna y el valor de cada posición
        const Column& col = table.columns[i];
        const std::string& value = values[i];

        switch (col.type)
        {
        case TYPE_INTEGER: {
            // convertir el string a entero y copiarlo como bytes
            int32_t num = std::stoi(value);
            memcpy(buffer + col.offset, &num, sizeof(int32_t));
            break;
        }
        case TYPE_DOUBLE: {
            // convertir el string a double y copiarlo como bytes
            double num = std::stod(value);
            memcpy(buffer + col.offset, &num, sizeof(double));
            break;
        }
        case TYPE_VARCHAR: {
            // copiar el string caracter por caracter hasta el tamanio maximo (copia segura de string a buffer)
            strncpy(buffer + col.offset, value.c_str(), col.size);
            break;
        }
        case TYPE_DATETIME: {
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
        default: {
            break;
        }
        }
    }
    return buffer;
}
