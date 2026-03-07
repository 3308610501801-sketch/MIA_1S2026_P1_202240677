#include "mount.h"
#include <sstream>

const std::string MountManager::CARNET_SUFFIX = "77"; 

std::vector<MountedPartition> MountManager::mountedPartitions;

std::string MountManager::generateMountID(const std::string& diskPath) {
    std::map<std::string, char> diskLetter;
    std::map<std::string, int>  diskNextNum;

    char nextLetter = 'A';

    for (auto& mp : mountedPartitions) {
        if (!diskLetter.count(mp.diskPath)) {
            diskLetter[mp.diskPath]  = nextLetter++;
            diskNextNum[mp.diskPath] = 1;
        }
        diskNextNum[mp.diskPath]++;
    }

    char letter;
    int  number;

    if (!diskLetter.count(diskPath)) {
        letter = nextLetter;
        number = 1;
    } else {
        letter = diskLetter[diskPath];
        number = diskNextNum[diskPath];
    }

    return CARNET_SUFFIX + std::to_string(number) + letter;
}

MountedPartition* MountManager::findByID(const std::string& id) {
    for (auto& mp : mountedPartitions)
        if (mp.id == id) return &mp;
    return nullptr;
}

bool MountManager::isMounted(const std::string& diskPath, const std::string& partName) {
    for (auto& mp : mountedPartitions)
        if (mp.diskPath == diskPath && mp.partName == partName)
            return true;
    return false;
}

std::string MountManager::mount(const std::string& diskPath, const std::string& partName) {
    std::string newID = generateMountID(diskPath);
    mountedPartitions.push_back({newID, diskPath, partName});
    return newID;
}

std::string MountManager::listMounted() {
    std::ostringstream out;
    if (mountedPartitions.empty()) {
        out << "No hay particiones montadas.\n";
    } else {
        out << "Particiones montadas:\n";
        out << "------------------------------\n";
        for (auto& mp : mountedPartitions) {
            out << "  ID: "        << mp.id
                << " | Disco: "    << mp.diskPath
                << " | Partición: "<< mp.partName << "\n";
        }
    }
    return out.str();
}
