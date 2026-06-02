#include "httplib.h"
#include <iostream>

int main() {
    httplib::Server svr;

    svr.Post("/query", [](const httplib::Request& req,
        httplib::Response& res) {
            std::cout << "Consulta recibida: " << req.body << std::endl;
            res.set_content("{\"success\": true, \"message\": \"ok\"}",
                "application/json");
        });

    std::cout << "Servidor corriendo en puerto 8080..." << std::endl;

    svr.set_default_headers({
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
    {"Access-Control-Allow-Headers", "Content-Type"}
        });

    svr.Options("/query", [](const httplib::Request&,
        httplib::Response& res) {
            res.set_content("", "text/plain");
        });

    svr.listen("0.0.0.0", 8080);

    return 0;
}