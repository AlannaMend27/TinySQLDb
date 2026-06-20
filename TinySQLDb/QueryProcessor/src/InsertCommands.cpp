#define _CRT_SECURE_NO_WARNINGS
#include <sstream>
#include <algorithm>
#include "InsertCommands.h"

InsertCommands::InsertCommands(StoredDataManager& dataManager, SystemCatalog& catalog, IndexManager& indexManager)
    : Commands(dataManager, catalog), indexManager(indexManager)
{
    //
}

// Ejecuta INSERT INTO <tabla> VALUES(...)
void InsertCommands::executeInsert(QueryResult& result, const std::string& statement, const std::string& database)
{

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
        return;
    }

    // separar los valores guardandolos en un array y obteniendo la cantidad de valores obtenidos
    std::string values[MAX_COLUMNS];
    int valueCount = this->splitValues(body, values);

    // validad que se pueda registrar la fila en la tabla
    if (!validateAndRegisterTable(database, table, result, tableName, values, valueCount))
    {
        return; // result con el mensaje de error
    }

    // verificar duplicados en columnas indexadas
    if (!this->checkDuplicatesOnIndexes(table, values, tableName, result))
    {
        return;
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
        return;
    }

    // actualizar los indices activos con el nuevo valor
    this->updateIndexesAfterInsert(table, values, tableName);

    result.success = true;
    result.message = "1 fila insertada en '" + tableName + "'";
    return;
}

// Extrae el nombre de la tabla del statement
std::string InsertCommands::extractTableNameForRow(const std::string& statement)
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
bool InsertCommands::validateAndRegisterTable(const std::string& database, const Table& table, QueryResult& result, const std::string tableName, const std::string values[], const int valueCount)
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
std::string InsertCommands::extractValuesBody(const std::string& statement)
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
int InsertCommands::splitValues(const std::string& body, std::string values[])
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
        if (body[i] == '\'' )
        {
            // valor entre comillas — leer hasta la comilla de cierre
            i++; // saltar la comilla de apertura
            while (i < len && body[i] != '\'')
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
    std::cout << "splitValues extrajo " << count << " valores:" << std::endl;
    for (int i = 0; i < count; i++)
        std::cout << "  [" << i << "] = '" << values[i] << "'" << std::endl;
    return count;
}

char* InsertCommands::serializeRowValues(const Table& table, const std::string values[], const int rowSize)
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

        // llamar al metodo de la clase padre para parsear los valores y guardarlos en el buffer
        this->serializeSingleValue(buffer, col, value);
    }
    return buffer;
}


// verifica que no haya duplicados en columnas indexadas antes de insertar
bool InsertCommands::checkDuplicatesOnIndexes(const Table& table, const std::string values[], const std::string& tableName, QueryResult& result)
{
    for (int i = 0; i < (int)table.columnCount; i++)
    {
        if (this->indexManager.hasIndex(tableName, table.columns[i].name))
        {
            ActiveIndex* activeIndex = this->indexManager.getIndex(tableName, table.columns[i].name);

            // verificar duplicado segun el tipo de arbol activo
            bool isDuplicate = false;

            if (activeIndex->type == INDEX_BST && activeIndex->BST != nullptr)
            {
                // el indice es BST verificar en el arbol binario
                isDuplicate = activeIndex->BST->valueExists(values[i]);
            }
            else if (activeIndex->bTree != nullptr)
            {
                // el indice es BTREE — verificar en el arbol B
                isDuplicate = activeIndex->bTree->valueExists(values[i]);
            }

            if (isDuplicate)
            {
                result.success = false;
                result.message = "Error: valor duplicado en columna '" + table.columns[i].name + "' que tiene un indice";
                return false;
            }
        }
    }
    return true;
}

void InsertCommands::updateIndexesAfterInsert(const Table& table, const std::string values[], const std::string& tableName)
{
    // obtener la cantidad de filas para calcular la posicion de la nueva
    int rowCount = 0;
    this->dataManager.readAllRows(table, rowCount);

    for (int i = 0; i < (int)table.columnCount; i++)
    {
        if (this->indexManager.hasIndex(tableName, table.columns[i].name))
        {
            ActiveIndex* activeIndex = this->indexManager.getIndex(tableName, table.columns[i].name);

            // calcular la posicion en disco de la nueva fila
            long position = (long)(rowCount - 1) * table.rowSize;

            if (activeIndex->type == INDEX_BST && activeIndex->BST != nullptr)
            {
                // actualizar el arbol BST con el nuevo valor
                activeIndex->BST->insert(values[i], position);
            }
            else if (activeIndex->bTree != nullptr)
            {
                // actualizar el arbol BTREE con el nuevo valor
                activeIndex->bTree->insert(values[i], position);
            }
        }
    }
}