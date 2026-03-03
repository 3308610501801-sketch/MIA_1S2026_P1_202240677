#include "diskManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <vector>
#include <algorithm>

bool DiskManager::createDisk(const std::string& path, int size, char fit) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        std::cerr << "[DiskManager] No se pudo crear: " << path << "\n";
        return false;
    }

    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, BUFFER_SIZE);

    int written = 0;
    while (written < size) {
        int chunk = std::min(BUFFER_SIZE, size - written);
        file.write(buffer, chunk);
        written += chunk;
    }
    file.close();

    // Escribir MBR inicial
    MBR mbr;
    mbr.mbr_tamano         = size;
    mbr.mbr_fecha_creacion = std::time(nullptr);
    mbr.mbr_dsk_signature  = std::rand();
    mbr.dsk_fit            = fit;

    if (!writeMBR(path, mbr)) {
        std::cerr << "[DiskManager] Error al escribir MBR inicial.\n";
        return false;
    }

    std::cout << "[DiskManager] Disco creado: " << path
              << " (" << size << " bytes)\n";
    return true;
}

bool DiskManager::writeMBR(const std::string& diskPath, const MBR& mbr) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    file.seekp(0);
    file.write(reinterpret_cast<const char*>(&mbr), sizeof(MBR));
    return file.good();
}

bool DiskManager::readMBR(const std::string& diskPath, MBR& mbr) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return false;
    file.seekg(0);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    return file.good();
}

bool DiskManager::writeEBR(const std::string& diskPath, int byteOffset, const EBR& ebr) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    file.seekp(byteOffset);
    file.write(reinterpret_cast<const char*>(&ebr), sizeof(EBR));
    return file.good();
}

bool DiskManager::readEBR(const std::string& diskPath, int byteOffset, EBR& ebr) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return false;
    file.seekg(byteOffset);
    file.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
    return file.good();
}

static int firstFreeByteAfterMBR(const MBR& mbr) {
    int pos = static_cast<int>(sizeof(MBR));
    for (int i = 0; i < 4; i++) {
        const Partition& p = mbr.mbr_partitions[i];
        if (p.part_start != -1) {
            int end = p.part_start + p.part_s;
            if (end > pos) pos = end;
        }
    }
    return pos;
}

// Recopila los huecos libres entre particiones existentes
struct Gap { int start; int size; };
static std::vector<Gap> freeGaps(const MBR& mbr) {
    // Ordenar particiones por part_start
    std::vector<std::pair<int,int>> parts;
    for (int i = 0; i < 4; i++) {
        const Partition& p = mbr.mbr_partitions[i];
        if (p.part_start != -1)
            parts.push_back({p.part_start, p.part_start + p.part_s});
    }
    std::sort(parts.begin(), parts.end());

    std::vector<Gap> gaps;
    int cursor = static_cast<int>(sizeof(MBR));
    for (auto& pr : parts) {
        if (pr.first > cursor)
            gaps.push_back({cursor, pr.first - cursor});
        cursor = pr.second;
    }
    // Hueco al final
    if (cursor < mbr.mbr_tamano)
        gaps.push_back({cursor, mbr.mbr_tamano - cursor});
    return gaps;
}

int DiskManager::calcStartFirstFit(const MBR& mbr, int size) {
    for (auto& g : freeGaps(mbr))
        if (g.size >= size) return g.start;
    return -1;
}

int DiskManager::calcStartBestFit(const MBR& mbr, int size) {
    int best = -1, bestSize = INT_MAX;
    for (auto& g : freeGaps(mbr)) {
        if (g.size >= size && g.size < bestSize) {
            bestSize = g.size;
            best     = g.start;
        }
    }
    return best;
}

int DiskManager::calcStartWorstFit(const MBR& mbr, int size) {
    int worst = -1, worstSize = -1;
    for (auto& g : freeGaps(mbr)) {
        if (g.size >= size && g.size > worstSize) {
            worstSize = g.size;
            worst     = g.start;
        }
    }
    return worst;
}

int DiskManager::findPartition(const MBR& mbr, const std::string& name) {
    for (int i = 0; i < 4; i++)
        if (std::string(mbr.mbr_partitions[i].part_name) == name)
            return i;
    return -1;
}


//  ----------[addPartition]----------

bool DiskManager::addPartition(const std::string& diskPath,
                                const std::string& name,
                                int size, char type, char fit) {
    MBR mbr;
    if (!readMBR(diskPath, mbr)) {
        std::cerr << "[DiskManager] No se pudo leer MBR.\n";
        return false;
    }

    // Buscar slot libre
    int slot = -1;
    for (int i = 0; i < 4; i++)
        if (mbr.mbr_partitions[i].part_start == -1) { slot = i; break; }

    if (slot == -1) {
        std::cerr << "[DiskManager] Disco lleno: ya hay 4 particiones.\n";
        return false;
    }

    // Verificar nombre único
    if (findPartition(mbr, name) != -1) {
        std::cerr << "[DiskManager] Ya existe una partición con ese nombre.\n";
        return false;
    }

    // Calcular inicio según ajuste
    int start = -1;
    switch (fit) {
        case 'B': start = calcStartBestFit (mbr, size); break;
        case 'W': start = calcStartWorstFit(mbr, size); break;
        default:  start = calcStartFirstFit(mbr, size); break;
    }

    if (start == -1) {
        std::cerr << "[DiskManager] No hay espacio suficiente para la partición.\n";
        return false;
    }

    // Llenar la entrada
    Partition& p   = mbr.mbr_partitions[slot];
    p.part_status  = '0';
    p.part_type    = type;
    p.part_fit     = fit;
    p.part_start   = start;
    p.part_s       = size;
    std::strncpy(p.part_name, name.c_str(), 15);
    p.part_name[15]     = '\0';
    p.part_correlative  = -1;

    // escribir el primer EBR al inicio de la partición
    if (type == 'E') {
        EBR firstEBR;
        firstEBR.part_fit   = fit;
        firstEBR.part_start = start + static_cast<int>(sizeof(EBR));
        firstEBR.part_s     = 0;   
        firstEBR.part_next  = -1;
        writeEBR(diskPath, start, firstEBR);
    }

    return writeMBR(diskPath, mbr);
}


//  ----------[deletePartition]----------
bool DiskManager::deletePartition(const std::string& diskPath,
                                   const std::string& name) {
    MBR mbr;
    if (!readMBR(diskPath, mbr)) return false;

    int idx = findPartition(mbr, name);
    if (idx == -1) {
        std::cerr << "[DiskManager] Partición no encontrada: " << name << "\n";
        return false;
    }

    // Limpiar la entrada
    mbr.mbr_partitions[idx] = Partition();
    return writeMBR(diskPath, mbr);
}

bool DiskManager::addLogicalUnit(const std::string& diskPath,
                                  const std::string& partExtName,
                                  const std::string& logicalName,
                                  int size, char fit) {
    MBR mbr;
    if (!readMBR(diskPath, mbr)) return false;

    int idx = findPartition(mbr, partExtName);
    if (idx == -1 || mbr.mbr_partitions[idx].part_type != 'E') {
        std::cerr << "[DiskManager] Partición extendida no encontrada.\n";
        return false;
    }

    Partition& ext = mbr.mbr_partitions[idx];
    int extEnd     = ext.part_start + ext.part_s;

    // Recorrer EBRs
    int ebrPos = ext.part_start;
    EBR ebr;
    while (true) {
        if (!readEBR(diskPath, ebrPos, ebr)) return false;
        if (ebr.part_next == -1) break;
        ebrPos = ebr.part_next;
    }

    int logStart = ebrPos + static_cast<int>(sizeof(EBR));
    int nextEBRPos = logStart + size;

    // Verificar que cabe dentro de la partición extendida
    if (nextEBRPos > extEnd) {
        std::cerr << "[DiskManager] No hay espacio en la partición extendida.\n";
        return false;
    }

    ebr.part_s    = size;
    ebr.part_fit  = fit;
    std::strncpy(ebr.part_name, logicalName.c_str(), 15);
    ebr.part_name[15] = '\0';
    ebr.part_next = nextEBRPos;
    writeEBR(diskPath, ebrPos, ebr);

    EBR newEBR;
    newEBR.part_fit   = fit;
    newEBR.part_start = nextEBRPos + static_cast<int>(sizeof(EBR));
    newEBR.part_s     = 0;
    newEBR.part_next  = -1;
    writeEBR(diskPath, nextEBRPos, newEBR);

    std::cout << "[DiskManager] Unidad lógica '" << logicalName
              << "' añadida en byte " << logStart << "\n";
    return true;
}
