#define _CRT_SECURE_NO_WARNINGS
#include "httplib.h"
#include "../QueryProcessor/include/QueryProcessor.h"
#include <iostream>
#include <string>

int main()
{
    // Inicializamos el procesador de consultas
    QueryProcessor processor;

    // crear el servidor HTTP
    httplib::Server svr;

    // crear headers CORS (para que react se conecte desde otro puerto)
    // Access-Control-Allow-Origin: "*" permite desde cualquier origen
    // Access-Control-Allow-Methods: Métodos HTTP permitidos
    // Access-Control-Allow-Headers: Headers permitidos en la petición
    svr.set_default_headers({
        {"Access-Control-Allow-Origin",  "*"},
        {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
        });

    // Manejo de peticiones con options, para verificar que el server permite cors
    svr.Options("/query", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("", "text/plain");
        });

    // Manejo de peticiones con POST (recibir y enviar datos)
    svr.Post("/query", [&processor](const httplib::Request& req, httplib::Response& res) {

        // cuerpo de la sentencia SQL recibida
        std::string body = req.body;

        // extraer statement
        std::string statement = "";
        int stmtStart = (int)body.find("\"statement\":");
        if (stmtStart != -1)
        {
            stmtStart = (int)body.find('"', stmtStart + 12) + 1;
            int stmtEnd = (int)body.find('"', stmtStart);
            statement = body.substr(stmtStart, stmtEnd - stmtStart);
        }

        // extraer database
        std::string database = "";
        int dbStart = (int)body.find("\"database\":");
        if (dbStart != -1)
        {
            dbStart = (int)body.find('"', dbStart + 11) + 1;
            int dbEnd = (int)body.find('"', dbStart);
            database = body.substr(dbStart, dbEnd - dbStart);
        }

        // ejecutar la sentencia
        QueryResult result;
        processor.execute(result, statement, database);

        // construir JSON de respuesta
        std::string json = "{";

        // campo success (bool)
        json += "\"success\":" + std::string(result.success ? "true" : "false") + ",";

        // campo message (string)
        json += "\"message\":\"" + result.message + "\",";

        // Campo timeMs (número)
        json += "\"timeMs\":" + std::to_string(result.timeMs) + ",";

        // Construir array de columnas obtenidas en el resultado para agregar al JSON
        json += "\"columns\":[";
        for (int i = 0; i < result.columnCount; i++)
        {
            // agregar coma entre elementos a excepcion del primero
            if (i > 0) 
            {
                json += ",";
            }
            // colocar comillas dobles a los datos, para que sean string
            json += "\"" + result.columnNames[i] + "\"";
        }
        json += "],";

        // Construir array de filas (matriz) para almacenarlo en el JSON y renderizar el resultado en la web
        json += "\"rows\":[";
        for (int i = 0; i < result.rowCount; i++)
        {
            // agregar coma entre elementos a excepcion del primero
            if (i > 0) 
            {
                json += ",";
            }

            json += "[";
            for (int j = 0; j < result.columnCount; j++)
            {
                // agregar coma entre celdas
                if (j > 0) {
                    json += ",";
                }

                // colocar comillas dobles a los datos, para que sean string
                json += "\"" + result.rows[i][j] + "\"";
            }
            json += "]";
        }
        json += "]";

        json += "}";

        // enviar el json que contruimos como respuesta al cliente
        res.set_content(json, "application/json");

        // verificar que datos extrae (esto lo quitamos al final son pruebitas)
        std::cout << "Body recibido: " << body << std::endl;
        std::cout << "Statement extraido: " << statement << std::endl;
        std::cout << "Database extraida: " << database << std::endl;

        });

    // iniciar el servidor 
    std::cout << "Servidor TinySQLDb corriendo en puerto 8080..." << std::endl;

    // poner al servidor a escuhar en el puerto 8080
    svr.listen("0.0.0.0", 8080);

    return 0;
}