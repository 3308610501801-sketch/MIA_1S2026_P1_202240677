#pragma once
#include <string>
#include <vector>
#include <map>

struct MountedPartition {
    std::string id;       
    std::string diskPath;  
    std::string partName;  
};

class MountManager {
public:
    static const std::string CARNET_SUFFIX;
    static std::vector<MountedPartition> mountedPartitions;
    static std::string generateMountID(const std::string& diskPath);
    static MountedPartition* findByID(const std::string& id);
    static bool isMounted(const std::string& diskPath, const std::string& partName);
    static std::string mount(const std::string& diskPath, const std::string& partName);
    static std::string listMounted();
};
