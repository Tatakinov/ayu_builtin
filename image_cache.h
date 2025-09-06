#ifndef IMAGE_CACHE_H_
#define IMAGE_CACHE_H_

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

class ImageInfo {
    private:
        std::vector<unsigned char> data_;
        int width_, height_;
        bool is_upconverted_;
    public:
        ImageInfo(const std::vector<unsigned char> &data, int width, int height, bool is_upconverted) : data_(data), width_(width), height_(height), is_upconverted_(is_upconverted) {}
        ~ImageInfo() {}
        const std::vector<unsigned char> &get() const {
            return data_;
        }
        int width() const {
            return width_;
        }
        int height() const {
            return height_;
        }
        bool is_upconverted() const {
            return is_upconverted_;
        }
};

class ImageCache {
    private:
        bool alive_;
        bool use_self_alpha_;
        int scale_;
        std::mutex mutex_;
        std::condition_variable cond_;
        std::unique_ptr<std::thread> th_;
        std::queue<std::filesystem::path> queue_;
        std::unordered_map<std::filesystem::path, std::optional<ImageInfo>> cache_orig_;
        std::unordered_map<std::filesystem::path, std::optional<ImageInfo>> cache_;
        Ort::Env env_;
        Ort::Session session_;

        const std::optional<ImageInfo> &getOriginal(const std::filesystem::path &path);

    public:
        ImageCache(const std::filesystem::path &exe_dir, bool use_self_alpha);
        ~ImageCache();
        void setScale(int scale);
        const std::optional<ImageInfo> &get(const std::filesystem::path &path);
        void clearCache();
};

#endif // IMAGE_CACHE_H_
