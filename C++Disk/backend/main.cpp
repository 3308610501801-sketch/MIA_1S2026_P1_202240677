#include "external/crow_all.h"
#include "utils/diskManager.h"
#include "utils/ext2.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

static std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return "";
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

static bool terminaCon(const std::string& s, const std::string& suf) {
    if (s.size() < suf.size()) return false;
    return s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static std::string contentType(const std::string& path) {
    if (terminaCon(path, ".html")) return "text/html";
    if (terminaCon(path, ".css"))  return "text/css";
    if (terminaCon(path, ".js"))   return "application/javascript";
    if (terminaCon(path, ".png"))  return "image/png";
    if (terminaCon(path, ".jpg") || terminaCon(path, ".jpeg")) return "image/jpeg";
    return "text/plain";
}

static std::string toLowerCase(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

struct ParsedCommand {
    std::string name;
    std::map<std::string, std::string> params;
};

static ParsedCommand parseCommand(const std::string& line) {
    ParsedCommand cmd;
    std::istringstream iss(line);
    std::string token;

    bool first = true;
    while (iss >> token) {
        if (first) {
            cmd.name = toLowerCase(token);
            first = false;
            continue;
        }
 
        if (token[0] == '-') {
            auto eq = token.find('=');
            if (eq == std::string::npos) continue;
            std::string key = toLowerCase(token.substr(1, eq - 1));
            std::string val = token.substr(eq + 1);

            // Eliminar comillas si las hay
            if (!val.empty() && val.front() == '"') {
                val = val.substr(1);
                // Continuar leyendo si la cadena no cerró
                while (val.empty() || val.back() != '"') {
                    std::string extra;
                    if (!(iss >> extra)) break;
                    val += " " + extra;
                }
                if (!val.empty() && val.back() == '"')
                    val.pop_back();
            }
            cmd.params[key] = val;
        }
    }
    return cmd;
}

static std::string ejecutarComando(const ParsedCommand& cmd) {
    std::ostringstream out;

    // ── MKDISK ──────────────────────────────────────────────
    if (cmd.name == "mkdisk") {
        int  size = 0;
        char unit = 'M';
        char fit  = 'F';
        std::string path;

        if (cmd.params.count("size"))  size = std::stoi(cmd.params.at("size"));
        if (cmd.params.count("unit"))  unit = std::toupper(cmd.params.at("unit")[0]);
        if (cmd.params.count("fit"))   fit  = std::toupper(cmd.params.at("fit")[0]);
        if (cmd.params.count("path"))  path = cmd.params.at("path");

        if (path.empty() || size <= 0) {
            return "Error: mkdisk requiere -path y -size.\n";
        }

        int bytes = (unit == 'K') ? size * 1024 : size * 1024 * 1024;

        if (DiskManager::createDisk(path, bytes, fit))
            out << "Disco creado exitosamente: " << path
                << " (" << size << (unit=='K'?"K":"M") << "B)\n";
        else
            out << "Error al crear el disco.\n";
    }

    else if (cmd.name == "rmdisk") {
        std::string path;
        if (cmd.params.count("path")) path = cmd.params.at("path");
        if (path.empty()) return "Error: rmdisk requiere -path.\n";

        if (std::remove(path.c_str()) == 0)
            out << "Disco eliminado: " << path << "\n";
        else
            out << "Error: no se pudo eliminar " << path << "\n";
    }

    else if (cmd.name == "fdisk") {
        int  size = 0;
        char unit = 'K';
        char fit  = 'F';
        char type = 'P';
        std::string path, name;

        if (cmd.params.count("size"))  size = std::stoi(cmd.params.at("size"));
        if (cmd.params.count("unit"))  unit = std::toupper(cmd.params.at("unit")[0]);
        if (cmd.params.count("fit"))   fit  = std::toupper(cmd.params.at("fit")[0]);
        if (cmd.params.count("type"))  type = std::toupper(cmd.params.at("type")[0]);
        if (cmd.params.count("path"))  path = cmd.params.at("path");
        if (cmd.params.count("name"))  name = cmd.params.at("name");

        if (path.empty() || name.empty() || size <= 0)
            return "Error: fdisk requiere -path, -name y -size.\n";

        int bytes = (unit == 'M') ? size * 1024 * 1024 : size * 1024;

        if (DiskManager::addPartition(path, name, bytes, type, fit))
            out << "Partición '" << name << "' creada en " << path << "\n";
        else
            out << "Error al crear la partición.\n";
    }

    else if (cmd.name == "mkfs") {
        std::string id, type;
        if (cmd.params.count("id"))   id   = cmd.params.at("id");
        if (cmd.params.count("type")) type = cmd.params.at("type");

        out << "[mkfs] Comando recibido. ID=" << id
            << " Tipo=" << type << "\n"
            << "NOTA: mkfs requiere que la partición esté montada (mount).\n";
    }

    else {
        out << "Comando no reconocido: " << cmd.name << "\n";
    }

    return out.str();
}

static std::string procesarTexto(const std::string& texto) {
    std::ostringstream resultado;
    std::istringstream iss(texto);
    std::string linea;

    while (std::getline(iss, linea)) {
        auto start = linea.find_first_not_of(" \t\r");
        if (start == std::string::npos) continue;
        if (linea[start] == '#') {
            resultado << linea << "\n";
            continue;
        }

        resultado << "> " << linea << "\n";
        ParsedCommand cmd = parseCommand(linea.substr(start));
        resultado << ejecutarComando(cmd);
    }

    return resultado.str();
}


int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        crow::response res;
        res.set_header("Content-Type", "text/html");
        res.body = leerArchivo("../frontend/index.html");
        return res;
    });

    CROW_ROUTE(app, "/<path>")
    ([](const std::string& path) {
        std::string fullPath = "../frontend/" + path;
        std::string body = leerArchivo(fullPath);
        if (body.empty()) return crow::response(404);
        crow::response res;
        res.set_header("Content-Type", contentType(fullPath));
        res.body = body;
        return res;
    });

    // ANALIZAR / EJECUTAR COMANDOS
    CROW_ROUTE(app, "/analizar")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        std::string texto = req.body;
        if (texto.empty())
            return crow::response("No se recibió texto.\n");

        std::string salida = procesarTexto(texto);
        return crow::response(salida);
    });

    std::cout << "Servidor iniciado en http://localhost:8080\n";
    app.port(8080).multithreaded().run();
}
