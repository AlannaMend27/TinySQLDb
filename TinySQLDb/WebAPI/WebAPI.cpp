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
    svr.listen("0.0.0.0", 8080);

    return 0;
}