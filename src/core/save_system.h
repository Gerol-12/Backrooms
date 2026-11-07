#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ModifiedBlock { int32_t x=0, y=0, z=0; uint8_t blockType=0; };
struct SaveData {
    uint32_t worldSeed=0;
    glm::vec3 playerPosition{16.5f, 1.0f, 16.5f};
    float yaw=-90.0f, pitch=0.0f;
    std::vector<ModifiedBlock> modifiedBlocks;
};

class SaveSystem {
public:
    static bool Save(const SaveData& data, const std::string& path="save.dat") {
        constexpr char magic[8]={'B','R','S','A','V','E','0','1'};
        constexpr uint32_t version=1, maxBlocks=5'000'000;
        if (data.modifiedBlocks.size()>maxBlocks ||
            !std::isfinite(data.playerPosition.x) || !std::isfinite(data.playerPosition.y) ||
            !std::isfinite(data.playerPosition.z) || !std::isfinite(data.yaw) ||
            !std::isfinite(data.pitch)) return false;
        const std::string tempPath=path+".tmp";
        std::ofstream file(tempPath,std::ios::binary|std::ios::trunc);
        if (!file) return false;
        const uint32_t count=static_cast<uint32_t>(data.modifiedBlocks.size());
        file.write(magic,sizeof(magic));
        write(file,version); write(file,data.worldSeed);
        write(file,data.playerPosition.x); write(file,data.playerPosition.y);
        write(file,data.playerPosition.z); write(file,data.yaw); write(file,data.pitch);
        write(file,count);
        for (const auto& b:data.modifiedBlocks) {
            write(file,b.x); write(file,b.y); write(file,b.z); write(file,b.blockType);
        }
        file.flush();
        if (!file.good()) { file.close(); return false; }
        file.close();
        return std::rename(tempPath.c_str(),path.c_str())==0;
    }

    static bool Load(SaveData& data,const std::string& path="save.dat") {
        constexpr char expected[8]={'B','R','S','A','V','E','0','1'};
        constexpr uint32_t versionExpected=1, maxBlocks=5'000'000;
        std::ifstream file(path,std::ios::binary);
        if (!file) return false;
        char magic[8]{}; uint32_t version=0,count=0; SaveData loaded;
        file.read(magic,sizeof(magic));
        if (!file || !std::equal(std::begin(magic),std::end(magic),std::begin(expected))) return false;
        if (!read(file,version) || version!=versionExpected ||
            !read(file,loaded.worldSeed) || !read(file,loaded.playerPosition.x) ||
            !read(file,loaded.playerPosition.y) || !read(file,loaded.playerPosition.z) ||
            !read(file,loaded.yaw) || !read(file,loaded.pitch) ||
            !read(file,count) || count>maxBlocks) return false;
        if (!std::isfinite(loaded.playerPosition.x) || !std::isfinite(loaded.playerPosition.y) ||
            !std::isfinite(loaded.playerPosition.z) || !std::isfinite(loaded.yaw) ||
            !std::isfinite(loaded.pitch)) return false;
        loaded.modifiedBlocks.resize(count);
        for (auto& b:loaded.modifiedBlocks)
            if (!read(file,b.x) || !read(file,b.y) || !read(file,b.z) || !read(file,b.blockType)) return false;
        data=std::move(loaded);
        return true;
    }
private:
    template<class T> static void write(std::ofstream& f,const T& v) {
        f.write(reinterpret_cast<const char*>(&v),sizeof(T));
    }
    template<class T> static bool read(std::ifstream& f,T& v) {
        return static_cast<bool>(f.read(reinterpret_cast<char*>(&v),sizeof(T)));
    }
};
