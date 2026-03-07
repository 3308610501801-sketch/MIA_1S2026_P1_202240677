#include "external/crow_all.h"
#include "utils/diskManager.h"
#include "utils/ext2.h"
#include "utils/mount.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>  
#include <filesystem>   
#include <cstring>      

namespace fs = std::filesystem;

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

    //  MKDISK 
    if (cmd.name == "mkdisk") {
        // Verificar parámetros
        for (auto& p : cmd.params) {
            if (p.first != "size" && p.first != "fit" &&
                p.first != "unit" && p.first != "path") {
                return "Error: parámetro no reconocido en mkdisk: -" + p.first + "\n";
            }
        }

        // -size (obligatorio)
        if (!cmd.params.count("size"))
            return "Error: mkdisk requiere el parámetro -size.\n";

        int size = 0;
        try { size = std::stoi(cmd.params.at("size")); }
        catch (...) { return "Error: -size debe ser un número entero.\n"; }
        if (size <= 0)
            return "Error: -size debe ser positivo y mayor que cero.\n";

        // -path (obligatorio)
        if (!cmd.params.count("path"))
            return "Error: mkdisk requiere el parámetro -path.\n";
        std::string path = cmd.params.at("path");

        // -unit (opcional, default M)
        char unit = 'M';
        if (cmd.params.count("unit")) {
            std::string u = cmd.params.at("unit");
            std::transform(u.begin(), u.end(), u.begin(), ::toupper);
            if (u != "K" && u != "M")
                return "Error: -unit solo acepta K o M.\n";
            unit = u[0];
        }

        // -fit (opcional, default FF)
        char fit = 'F';
        if (cmd.params.count("fit")) {
            std::string f = cmd.params.at("fit");
            std::transform(f.begin(), f.end(), f.begin(), ::toupper);
            if      (f == "BF") fit = 'B';
            else if (f == "FF") fit = 'F';
            else if (f == "WF") fit = 'W';
            else return "Error: -fit solo acepta BF, FF o WF.\n";
        }

        // Crear carpetas del path si no existen
        fs::path diskPath(path);
        if (diskPath.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(diskPath.parent_path(), ec);
            if (ec)
                return "Error: no se pudo crear la ruta: " + diskPath.parent_path().string() + "\n";
        }

        int bytes = (unit == 'K') ? size * 1024 : size * 1024 * 1024;

        if (DiskManager::createDisk(path, bytes, fit))
            out << "Disco creado exitosamente: " << path
                << " (" << size << (unit == 'K' ? "K" : "M") << "B)\n";
        else
            out << "Error: no se pudo crear el disco en " << path << "\n";
    }

    // -- RMDISK 
    else if (cmd.name == "rmdisk") {

        for (auto& p : cmd.params) {
            if (p.first != "path")
                return "Error: parámetro no reconocido en rmdisk: -" + p.first + "\n";
        }

        if (!cmd.params.count("path"))
            return "Error: rmdisk requiere el parámetro -path.\n";

        std::string path = cmd.params.at("path");

        // Verificar que el archivo existe
        if (!fs::exists(path))
            return "Error: el archivo no existe: " + path + "\n";

        if (std::remove(path.c_str()) == 0)
            out << "Disco eliminado exitosamente: " << path << "\n";
        else
            out << "Error: no se pudo eliminar " << path << "\n";
    }

    // -- FDISK -----------------------------------------------
    else if (cmd.name == "fdisk") {
        // Verificar parámetros no reconocidos
        for (auto& p : cmd.params) {
            if (p.first != "size" && p.first != "unit" && p.first != "path" &&
                p.first != "type" && p.first != "fit" && p.first != "name")
                return "Error: parámetro no reconocido en fdisk: -" + p.first + "\n";
        }

        // -size (obligatorio al crear)
        if (!cmd.params.count("size"))
            return "Error: fdisk requiere el parámetro -size.\n";
        int size = 0;
        try { size = std::stoi(cmd.params.at("size")); }
        catch (...) { return "Error: -size debe ser un número entero.\n"; }
        if (size <= 0)
            return "Error: -size debe ser positivo y mayor que cero.\n";

        // -path (obligatorio)
        if (!cmd.params.count("path"))
            return "Error: fdisk requiere el parámetro -path.\n";
        std::string path = cmd.params.at("path");
        if (!fs::exists(path))
            return "Error: el disco no existe: " + path + "\n";

        // -name (obligatorio)
        if (!cmd.params.count("name"))
            return "Error: fdisk requiere el parámetro -name.\n";
        std::string name = cmd.params.at("name");

        // -unit (opcional, default K)
        char unit = 'K';
        if (cmd.params.count("unit")) {
            std::string u = cmd.params.at("unit");
            std::transform(u.begin(), u.end(), u.begin(), ::toupper);
            if      (u == "B") unit = 'B';
            else if (u == "K") unit = 'K';
            else if (u == "M") unit = 'M';
            else return "Error: -unit solo acepta B, K o M.\n";
        }

        // -type (opcional, default P)
        char type = 'P';
        if (cmd.params.count("type")) {
            std::string t = cmd.params.at("type");
            std::transform(t.begin(), t.end(), t.begin(), ::toupper);
            if      (t == "P") type = 'P';
            else if (t == "E") type = 'E';
            else if (t == "L") type = 'L';
            else return "Error: -type solo acepta P, E o L.\n";
        }

        // -fit (opcional, default WF)
        char fit = 'W';
        if (cmd.params.count("fit")) {
            std::string f = cmd.params.at("fit");
            std::transform(f.begin(), f.end(), f.begin(), ::toupper);
            if      (f == "BF") fit = 'B';
            else if (f == "FF") fit = 'F';
            else if (f == "WF") fit = 'W';
            else return "Error: -fit solo acepta BF, FF o WF.\n";
        }

        // Calcular bytes según unidad
        int bytes = size;
        if      (unit == 'K') bytes = size * 1024;
        else if (unit == 'M') bytes = size * 1024 * 1024;

        // Tipo L   usar addLogicalUnit
        if (type == 'L') {
            // Buscar la partición extendida en el disco
            MBR mbr;
            if (!DiskManager::readMBR(path, mbr))
                return "Error: no se pudo leer el disco.\n";

            std::string extName = "";
            bool hayExtendida = false;
            for (int i = 0; i < 4; i++) {
                if (mbr.mbr_partitions[i].part_type == 'E' &&
                    mbr.mbr_partitions[i].part_start != -1) {
                    extName     = mbr.mbr_partitions[i].part_name;
                    hayExtendida = true;
                    break;
                }
            }
            if (!hayExtendida)
                return "Error: no existe una partición extendida en el disco. No se puede crear una lógica.\n";

            if (DiskManager::addLogicalUnit(path, extName, name, bytes, fit))
                out << "Partición lógica '" << name << "' creada en " << path << "\n";
            else
                out << "Error: no se pudo crear la partición lógica. Verifique el espacio disponible.\n";
        }
        else {
            // Tipo P o E   addPartition
            if (DiskManager::addPartition(path, name, bytes, type, fit))
                out << "Partición '" << name << "' ("
                    << (type == 'P' ? "Primaria" : "Extendida")
                    << ") creada en " << path << "\n";
            else
                out << "Error: no se pudo crear la partición. Verifique espacio, límite de 4 particiones (P+E) o nombre duplicado.\n";
        }
    }

    // -- MOUNT -----------------------------------------------
    else if (cmd.name == "mount") {
        for (auto& p : cmd.params) {
            if (p.first != "path" && p.first != "name")
                return "Error: parámetro no reconocido en mount: -" + p.first + "\n";
        }
        if (!cmd.params.count("path"))
            return "Error: mount requiere el parámetro -path.\n";
        if (!cmd.params.count("name"))
            return "Error: mount requiere el parámetro -name.\n";

        std::string path     = cmd.params.at("path");
        std::string partName = cmd.params.at("name");

        if (!fs::exists(path))
            return "Error: el disco no existe: " + path + "\n";

        // Verificar que la partición existe y es primaria
        MBR mbr;
        if (!DiskManager::readMBR(path, mbr))
            return "Error: no se pudo leer el disco.\n";

        int idx = -1;
        for (int i = 0; i < 4; i++) {
            if (std::string(mbr.mbr_partitions[i].part_name) == partName) {
                idx = i; break;
            }
        }
        if (idx == -1)
            return "Error: la partición '" + partName + "' no existe en " + path + "\n";
        if (mbr.mbr_partitions[idx].part_type != 'P')
            return "Error: solo se pueden montar particiones primarias.\n";

        // Verificar que no esté ya montada
        if (MountManager::isMounted(path, partName))
            return "Error: la partición '" + partName + "' ya está montada.\n";

        // Generar ID y registrar en memoria
        std::string newID = MountManager::mount(path, partName);

        // Calcular correlativo (cuántas particiones de este disco hay montadas)
        int correlativo = 0;
        for (auto& mp : MountManager::mountedPartitions)
            if (mp.diskPath == path) correlativo++;

        // Actualizar MBR en disco
        mbr.mbr_partitions[idx].part_status      = '1';
        mbr.mbr_partitions[idx].part_correlative = correlativo;
        std::strncpy(mbr.mbr_partitions[idx].part_id, newID.c_str(), 3);
        mbr.mbr_partitions[idx].part_id[3] = '\0';
        DiskManager::writeMBR(path, mbr);

        out << "Partición montada exitosamente.\n"
            << "  ID:        " << newID    << "\n"
            << "  Disco:     " << path     << "\n"
            << "  Partición: " << partName << "\n";
    }

    // -- MOUNTED ---------------------------------------------
    else if (cmd.name == "mounted") {
        if (!cmd.params.empty())
            return "Error: mounted no acepta parámetros.\n";
        out << MountManager::listMounted();
    }

    // -- MKFS ------------------------------------------------
    else if (cmd.name == "mkfs") {
        for (auto& p : cmd.params) {
            if (p.first != "id" && p.first != "type")
                return "Error: parámetro no reconocido en mkfs: -" + p.first + "\n";
        }
        if (!cmd.params.count("id"))
            return "Error: mkfs requiere el parámetro -id.\n";

        std::string id = cmd.params.at("id");

        // -type (opcional, default full)
        std::string type = "full";
        if (cmd.params.count("type")) {
            type = cmd.params.at("type");
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            if (type != "full")
                return "Error: -type solo acepta 'full'.\n";
        }

        // Buscar el ID en la tabla de montajes
        MountedPartition* target = MountManager::findByID(id);
        if (!target)
            return "Error: el ID '" + id + "' no existe. Use mount primero.\n";

        if (Ext2Manager::formatPartition(target->diskPath, target->partName))
            out << "Partición '" << target->partName << "' (ID: " << id
                << ") formateada con EXT2 exitosamente.\n";
        else
            out << "Error: no se pudo formatear la partición.\n";
    }

    // -- DESCONOCIDO -----------------------------------------
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
        // Ignorar comentarios y líneas vacías
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