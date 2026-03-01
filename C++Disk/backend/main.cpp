#include "external/crow_all.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string leerArchivo(const std::string& ruta) {
    std::ifstream archivo(ruta, std::ios::binary);
    if (!archivo) return "";
    std::stringstream buffer;
    buffer << archivo.rdbuf();
    return buffer.str();
}

bool terminaCon(const std::string& texto, const std::string& sufijo) {
    if (texto.length() < sufijo.length()) return false;
    return texto.compare(
        texto.length() - sufijo.length(),
        sufijo.length(),
        sufijo
    ) == 0;
}

std::string contentType(const std::string& path) {
    if (terminaCon(path, ".html")) return "text/html";
    if (terminaCon(path, ".css"))  return "text/css";
    if (terminaCon(path, ".js"))   return "application/javascript";
    if (terminaCon(path, ".png"))  return "image/png";
    if (terminaCon(path, ".jpg") || terminaCon(path, ".jpeg"))
        return "image/jpeg";
    return "text/plain";
}

int main() {
    crow::SimpleApp app;

    // INDEX
    CROW_ROUTE(app, "/")
    ([]() {
        crow::response res;
        res.set_header("Content-Type", "text/html");
        res.body = leerArchivo("../frontend/index.html");
        return res;
    });

    // ARCHIVOS ESTÁTICOS
    CROW_ROUTE(app, "/<path>")
    ([](const std::string& path) {
        std::string fullPath = "../frontend/" + path;
        std::string body = leerArchivo(fullPath);

        if (body.empty()) {
            return crow::response(404);
        }

        crow::response res;
        res.set_header("Content-Type", contentType(fullPath));
        res.body = body;
        return res;
    });

    CROW_ROUTE(app, "/analizar")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {

        std::string texto = req.body;

        std::cout << "Texto de entrada: " << texto << std::endl;

        // Respuesta al frontend
        return crow::response(
            "Texto recibido por el servidor:\n" + texto
        );
    });

    std::cout << "Servidor iniciado en http://localhost:8080\n";
    app.port(8080).multithreaded().run();
}