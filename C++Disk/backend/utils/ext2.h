#pragma once
#include <string>
#include "structs.h"

class Ext2Manager {
public:
    static bool formatPartition(const std::string& diskPath, const std::string& partName);
    static bool readSuperBlock(const std::string& diskPath, int partStart, SuperBlock& sb);
    static bool writeSuperBlock(const std::string& diskPath, int partStart, const SuperBlock& sb);
    static int  firstFreeInode(const std::string& diskPath, const SuperBlock& sb);
    static int  firstFreeBlock(const std::string& diskPath, const SuperBlock& sb);
    static bool setInodeBit (const std::string& diskPath, const SuperBlock& sb, int idx, char val);
    static bool setBlockBit (const std::string& diskPath, const SuperBlock& sb, int idx, char val);
    static bool writeInode(const std::string& diskPath, const SuperBlock& sb, int inodeIdx, const Inode& inode);
    static bool readInode (const std::string& diskPath, const SuperBlock& sb, int inodeIdx, Inode& inode);
    static bool writeFolderBlock(const std::string& diskPath, const SuperBlock& sb, int blockIdx, const FolderBlock& fb);
    static bool readFolderBlock (const std::string& diskPath, const SuperBlock& sb, int blockIdx, FolderBlock& fb);
    static bool writeFileBlock(const std::string& diskPath, const SuperBlock& sb, int blockIdx, const FileBlock& fb);
    static bool readFileBlock  (const std::string& diskPath, const SuperBlock& sb, int blockIdx, FileBlock& fb);

private:
    static int calcNumInodes(int partSize);
    static bool createRootDir(const std::string& diskPath, SuperBlock& sb);
};
