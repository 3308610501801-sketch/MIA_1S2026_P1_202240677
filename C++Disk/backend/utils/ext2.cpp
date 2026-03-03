#include "ext2.h"
#include "diskManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <ctime>

int Ext2Manager::calcNumInodes(int partSize) {
    // sizeof de las estructuras usadas en el cálculo
    int sb  = static_cast<int>(sizeof(SuperBlock));
    int si  = static_cast<int>(sizeof(Inode));
    int sb2 = static_cast<int>(sizeof(FolderBlock)); 

    double denom = 1.0 + 3.0 + static_cast<double>(si) + 3.0 * static_cast<double>(sb2);
    double n = static_cast<double>(partSize - sb) / denom;
    return static_cast<int>(std::floor(n));
}

bool Ext2Manager::readSuperBlock(const std::string& diskPath,
                                  int partStart, SuperBlock& sb) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return false;
    file.seekg(partStart);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    return file.good();
}

bool Ext2Manager::writeSuperBlock(const std::string& diskPath,
                                   int partStart, const SuperBlock& sb) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    file.seekp(partStart);
    file.write(reinterpret_cast<const char*>(&sb), sizeof(SuperBlock));
    return file.good();
}

static int firstFreeBit(const std::string& diskPath, int bitmapStart, int count) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return -1;
    file.seekg(bitmapStart);
    for (int i = 0; i < count; i++) {
        char bit;
        file.read(&bit, 1);
        if (bit == '0') return i;
    }
    return -1;
}

static bool setBit(const std::string& diskPath, int bitmapStart, int idx, char val) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    file.seekp(bitmapStart + idx);
    file.write(&val, 1);
    return file.good();
}

int Ext2Manager::firstFreeInode(const std::string& diskPath, const SuperBlock& sb) {
    return firstFreeBit(diskPath, sb.s_bm_inode_start, sb.s_inodes_count);
}

int Ext2Manager::firstFreeBlock(const std::string& diskPath, const SuperBlock& sb) {
    return firstFreeBit(diskPath, sb.s_bm_block_start, sb.s_blocks_count);
}

bool Ext2Manager::setInodeBit(const std::string& diskPath, const SuperBlock& sb,
                               int idx, char val) {
    return setBit(diskPath, sb.s_bm_inode_start, idx, val);
}

bool Ext2Manager::setBlockBit(const std::string& diskPath, const SuperBlock& sb,
                               int idx, char val) {
    return setBit(diskPath, sb.s_bm_block_start, idx, val);
}

//  ----------[Inodos]----------
bool Ext2Manager::writeInode(const std::string& diskPath, const SuperBlock& sb,
                              int inodeIdx, const Inode& inode) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    int offset = sb.s_inode_start + inodeIdx * static_cast<int>(sizeof(Inode));
    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&inode), sizeof(Inode));
    return file.good();
}

bool Ext2Manager::readInode(const std::string& diskPath, const SuperBlock& sb,
                             int inodeIdx, Inode& inode) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return false;
    int offset = sb.s_inode_start + inodeIdx * static_cast<int>(sizeof(Inode));
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));
    return file.good();
}

//  ----------[Bloques carpeta]----------
bool Ext2Manager::writeFolderBlock(const std::string& diskPath, const SuperBlock& sb,
                                    int blockIdx, const FolderBlock& fb) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    int offset = sb.s_block_start + blockIdx * static_cast<int>(sizeof(FolderBlock));
    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&fb), sizeof(FolderBlock));
    return file.good();
}

bool Ext2Manager::readFolderBlock(const std::string& diskPath, const SuperBlock& sb,
                                   int blockIdx, FolderBlock& fb) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return false;
    int offset = sb.s_block_start + blockIdx * static_cast<int>(sizeof(FolderBlock));
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&fb), sizeof(FolderBlock));
    return file.good();
}

//  ----------[Bloques archivo]----------
bool Ext2Manager::writeFileBlock(const std::string& diskPath, const SuperBlock& sb,
                                  int blockIdx, const FileBlock& fb) {
    std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    int offset = sb.s_block_start + blockIdx * static_cast<int>(sizeof(FileBlock));
    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&fb), sizeof(FileBlock));
    return file.good();
}

bool Ext2Manager::readFileBlock(const std::string& diskPath, const SuperBlock& sb,
                                 int blockIdx, FileBlock& fb) {
    std::ifstream file(diskPath, std::ios::binary);
    if (!file) return false;
    int offset = sb.s_block_start + blockIdx * static_cast<int>(sizeof(FileBlock));
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&fb), sizeof(FileBlock));
    return file.good();
}

bool Ext2Manager::createRootDir(const std::string& diskPath, SuperBlock& sb) {
    Inode root;
    root.i_uid   = 1;
    root.i_gid   = 1;
    root.i_size  = static_cast<int>(sizeof(FolderBlock));
    root.i_atime = root.i_ctime = root.i_mtime = static_cast<int>(std::time(nullptr));
    root.i_type  = 0;  
    root.i_perm  = 777;
    root.i_block[0] = 0; 

    FolderBlock fb;
    fb.b_content[0].inodo = 0; std::strcpy(fb.b_content[0].name, ".");
    fb.b_content[1].inodo = 0; std::strcpy(fb.b_content[1].name, "..");

    writeInode(diskPath, sb, 0, root);
    writeFolderBlock(diskPath, sb, 0, fb);

    setInodeBit(diskPath, sb, 0, '1');
    setBlockBit(diskPath, sb, 0, '1');

    sb.s_free_inodes_count--;
    sb.s_free_blocks_count--;
    sb.s_first_ino = 1;
    sb.s_first_blo = 1;

    return true;
}


//  ----------[formatPartition]----------
bool Ext2Manager::formatPartition(const std::string& diskPath,
                                   const std::string& partName) {
    // Leer MBR para encontrar la partición
    MBR mbr;
    if (!DiskManager::readMBR(diskPath, mbr)) {
        std::cerr << "[Ext2] No se pudo leer el MBR.\n";
        return false;
    }

    int idx = -1;
    for (int i = 0; i < 4; i++)
        if (std::string(mbr.mbr_partitions[i].part_name) == partName) { idx = i; break; }

    if (idx == -1) {
        std::cerr << "[Ext2] Partición no encontrada: " << partName << "\n";
        return false;
    }

    Partition& part = mbr.mbr_partitions[idx];
    if (part.part_type != 'P') {
        std::cerr << "[Ext2] Solo se puede formatear particiones primarias.\n";
        return false;
    }

    int partStart = part.part_start;
    int partSize  = part.part_s;

    // Calcular número de estructuras
    int n = calcNumInodes(partSize);
    if (n <= 0) {
        std::cerr << "[Ext2] Partición demasiado pequeña para EXT2.\n";
        return false;
    }

    int numInodes = n;
    int numBlocks = 3 * n;

    // Construir SuperBlock
    SuperBlock sb;
    sb.s_filesystem_type   = 2;
    sb.s_inodes_count      = numInodes;
    sb.s_blocks_count      = numBlocks;
    sb.s_free_inodes_count = numInodes;
    sb.s_free_blocks_count = numBlocks;
    sb.s_mtime             = static_cast<int>(std::time(nullptr));
    sb.s_umtime            = 0;
    sb.s_mnt_count         = 0;
    sb.s_magic             = 0xEF53;
    sb.s_inode_size        = static_cast<int>(sizeof(Inode));
    sb.s_block_size        = static_cast<int>(sizeof(FolderBlock));
    sb.s_first_ino         = 0;
    sb.s_first_blo         = 0;

    // Calcular posiciones absolutas en el disco
    int pos = partStart;
    pos += static_cast<int>(sizeof(SuperBlock));   

    sb.s_bm_inode_start = pos;   pos += numInodes;             
    sb.s_bm_block_start = pos;   pos += numBlocks;            
    sb.s_inode_start    = pos;   pos += numInodes * static_cast<int>(sizeof(Inode));
    sb.s_block_start    = pos;  

    // Escribir SuperBlock
    if (!writeSuperBlock(diskPath, partStart, sb)) {
        std::cerr << "[Ext2] Error al escribir SuperBlock.\n";
        return false;
    }

    // Inicializar bitmaps con '0'
    {
        std::fstream file(diskPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) return false;

        // Bitmap inodos
        file.seekp(sb.s_bm_inode_start);
        for (int i = 0; i < numInodes; i++) file.put('0');

        // Bitmap bloques
        file.seekp(sb.s_bm_block_start);
        for (int i = 0; i < numBlocks; i++) file.put('0');
    }

    // Crear directorio raíz
    createRootDir(diskPath, sb);
    writeSuperBlock(diskPath, partStart, sb);

    std::cout << "[Ext2] Partición '" << partName << "' formateada con EXT2.\n"
              << "       Inodos: " << numInodes << " | Bloques: " << numBlocks << "\n";
    return true;
}
