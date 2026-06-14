#pragma once
#include <string>
#include <algorithm>
#include <cstring>
#include <ctime>
#include "QueryResult.h"
#include "Table.h"
#include "Column.h"
#include "StoredDataManager.h"
#include "SystemCatalog.h"
#include <sstream>
#include <iostream>


// Commands -> esta es la clase padre de todos los comandos de bases de datos
// Contiene los metodos que comunmente se reutilizan entre los distintos comandos que se implementaron

class Commands {
public:
    // convierte los bytes de una columna a string legible
    static std::string deserializeValue(const char* buffer, const Column& col)
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
            struct tm timeInfoStruct;
            localtime_s(&timeInfoStruct, &t);

            // dar formato de string al tiempo con la funcion strftime
            char formatted[20];
            strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", &timeInfoStruct);

            // retornar string
            return std::string(formatted);
        }

        default:
            return "";

        }
    }

protected:

    // Atributos de dataManager y systemCatalog, los heredan todas las hijas
    StoredDataManager& dataManager;
    SystemCatalog& systemCatalog;

    // constructor protegido — solo las clases hijas lo llaman
    Commands(StoredDataManager& dataManager, SystemCatalog& catalog)
        : dataManager(dataManager), systemCatalog(catalog)
    {
        //
    }

    // metodos de validaciones con system catalog
     
    // valida que la base de datos y la tabla existan
    bool validateDBTable(const std::string& database, const std::string& tableName, QueryResult& result)
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

    // metodos de parseo
    
    // serializa un solo valor dentro del buffer en la posicion de su columna
    void serializeSingleValue(char* buffer, const Column& col, const std::string& value)
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
            strncpy_s(buffer + col.offset, col.size, value.c_str(), col.size);
            break;
        }
        case TYPE_DATETIME:
        {
            // parsear el string "YYYY-MM-DD HH:MM:SS" a timestamp unix

            // usamos struct tm para guardar cada parte de la fecha
            struct tm timeInfo = {};

            // sscanf lee los campos del string con formato de fecha
            sscanf_s(value.c_str(), "%d-%d-%d %d:%d:%d",
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

    // metodos relacionados con WHERE
    
    // extrae la condicion WHERE si existe
    bool parseWhere(const std::string& statement, std::string& whereColumn, std::string& whereOperator, std::string& whereValue)
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

    // verifica si una fila cumple la condicion WHERE
    bool rowMatchesWhere(const char* buffer, const Table & table, const std::string & whereColumn,
                         const std::string & whereOperator, const std::string & whereValue)
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
    
};