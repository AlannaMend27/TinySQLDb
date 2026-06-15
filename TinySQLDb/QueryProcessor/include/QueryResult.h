#pragma once
#include <string>
#include "Table.h"
// QueryResult -> struct que representa la respuesta del servidor al cliente
// contiene la informacion que WebAPI necesita para armar el JSON de respuesta

struct QueryResult {

    bool success; // indica si la sentencia se ejecuto bien
    std::string message;// mensaje de exito o error para el cliente
    long long timeMs; // tiempo en milisegundos que tardo el server

    //Para select
    std::string columnNames[MAX_COLUMNS];// nombres de las columnas seleccionadas
    int columnCount; // cantidad de columnas en el resultado
    std::string rows[MAX_ROWS][MAX_COLUMNS]; // valores de cada fila ya convertidos a string
    int rowCount; // cantidad de filas en el resultado

    // Constructor vacio, inicia todo limpio
    QueryResult()
    {
        this->success = false;
        this->message = "";
        this->timeMs = 0;
        this->columnCount = 0;
        this->rowCount = 0;
    }
};
