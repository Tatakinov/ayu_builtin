#ifndef SURFACES_H_
#define SURFACES_H_

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "seriko.h"
#include "surface.h"

class Seriko;

class Surfaces {
    private:
        int version_;
        std::unordered_map<int, Surface> surfaces_;
        std::unordered_map<std::string, std::vector<int>> alias_;
    public:
        Surfaces(const std::filesystem::path &ayu_dir);
        ~Surfaces() {}
        void addSurface(int n, const std::filesystem::path path) {
            surfaces_[n].element[0] = {Method::Base, 0, 0, path};
        }
        void parse(const std::filesystem::path &path);
        std::unique_ptr<Seriko> getSeriko() const;
        void dump() const;
};

#endif // SURFACES_H_
