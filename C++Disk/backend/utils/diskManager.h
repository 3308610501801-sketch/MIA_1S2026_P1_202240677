#pragma once
#include <string>
#include "structs.h"

class DiskManager {
public:
    static bool createDisk(const std::string& path, int size, char fit);
    static bool writeMBR(const std::string& diskPath, const MBR& mbr);
    static bool readMBR (const std::string& diskPath, MBR& mbr);
    static bool addPartition(const std::string& diskPath,
                             const std::string& name,
                             int size, char type, char fit);

    static bool deletePartition(const std::string& diskPath,
                                const std::string& name);
    static bool writeEBR(const std::string& diskPath, int byteOffset, const EBR& ebr);
    static bool readEBR (const std::string& diskPath, int byteOffset, EBR& ebr);
    static bool addLogicalUnit(const std::string& diskPath,
                               const std::string& partExtName,
                               const std::string& logicalName,
                               int size, char fit);

private:

    static int  calcStartBestFit (const MBR& mbr, int size);
    static int  calcStartFirstFit(const MBR& mbr, int size);
    static int  calcStartWorstFit(const MBR& mbr, int size);
    static int  findPartition(const MBR& mbr, const std::string& name);
};
