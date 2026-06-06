#pragma once
#include <string>

// QueryResult -> struct que representa la respuesta del servidor al cliente
// contiene la informacion que WebAPI necesita para armar el JSON de respuesta
struct QueryResult {

    bool success;          // indica si la sentencia se ejecuto bien
    std::string message;   // mensaje de exito o error para el cliente
    long long timeMs;      // tiempo en milisegundos que tardo el server

    // Constructor vacio, inicia todo limpio
    QueryResult()
    {
        this->success = false;
        this->message = "";
        this->timeMs = 0;
    }
};

// aqui se agrega despues lo demas para select y delete