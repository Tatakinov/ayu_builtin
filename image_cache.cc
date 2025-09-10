#include "image_cache.h"

#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#include "logger.h"

#if defined(USE_ONNX)
ImageCache::ImageCache(const std::filesystem::path &exe_dir, bool use_self_alpha)
    : alive_(true), use_self_alpha_(use_self_alpha), scale_(100), session_(nullptr) {
    std::filesystem::path model_path = exe_dir / "model.onnx";
    try {
        Ort::SessionOptions session_options;
        session_ = {env_, model_path.string().c_str(), session_options};
        th_ = std::make_unique<std::thread>([&]() {
            while (true) {
                std::filesystem::path p;
                int scale;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_.wait(lock, [&]() { return !queue_.empty(); });
                    p = queue_.front();
                    queue_.pop();
                    scale = scale_;
                }
                int num_resize = std::ceil(std::log2(scale_ / 100.0));
                auto &info = cache_orig_.at(p);
                int w = info->width();
                int h = info->height();
                std::vector<unsigned char> src;
                std::vector<unsigned char> dest = info->get();
                for (int i = 0; i < num_resize; i++, w <<= 1, h <<= 1) {
                    src = dest;
                    dest.resize(src.size() * 4);
                    std::array<int64_t, 4> input_shape = {4, 1, h, w};
                    std::array<int64_t, 4> output_shape = {4, 1, 2 * h, 2 * w};
                    std::vector<float> input;
                    input.resize(src.size());
                    std::vector<float> output;
                    output.resize(dest.size());
                    for (int i = 0; i < w * h; i++) {
                        for (int c = 0; c < 4; c++) {
                            input[c * w * h + i] = src[4 * i + c] / 255.0;
                        }
                    }
                    auto mem_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
                    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(mem_info, input.data(), input.size(), input_shape.data(), input_shape.size());
                    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(mem_info, output.data(), output.size(), output_shape.data(), output_shape.size());
                    const char *input_names[] = {"input"};
                    const char *output_names[] = {"output"};
                    Ort::RunOptions run_options;
                    try {
                        session_.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
                    }
                    catch (Ort::Exception &e) {
                        Logger::log(e.what());
                        auto &tmp = cache_.at(p);
                        cache_[p] = {tmp->get(), tmp->width(), tmp->height(), true};
                    }
                    for (int i = 0; i < (2 * w) * (2 * h); i++) {
                        for (int c = 0; c < 4; c++) {
                            int byte = std::round(output[c * (2 * w) * (2 * h) + i] * 255);
                            dest[4 * i + c] = std::max(0, std::min(255, byte));
                        }
                    }
                }
                if (info->width() * scale_ / 100.0 != w) {
                    int w_resize = std::round(info->width() * scale_ / 100.0);
                    int h_resize = std::round(info->height() * scale_ / 100.0);
                    std::vector<unsigned char> resize;
                    resize.resize(w_resize * h_resize * 4);
                    stbir_resize_uint8(dest.data(), w, h, 0, resize.data(), w_resize, h_resize, 0, 4);
                    dest = resize;
                    w = w_resize;
                    h = h_resize;
                }
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    if (scale == scale_) {
                        cache_[p] = {dest, w, h, true};
                    }
                }
                Logger::log("upconverted!");
            }
        });
    }
    catch (Ort::Exception &e) {
        Logger::log(e.what());
    }
}
#endif // USE_ONNX

ImageCache::~ImageCache() {
    alive_ = false;
    if (th_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push("");
            cond_.notify_one();
        }
        th_->join();
    }
}

void ImageCache::setScale(int scale) {
    std::unique_lock<std::mutex> lock(mutex_);
    scale_ = scale;
    cache_.clear();
}

const std::optional<ImageInfo> &ImageCache::getOriginal(const std::filesystem::path &path) {
    Logger::log("scale => ", scale_);
    if (cache_orig_.contains(path)) {
        return cache_.at(path);
    }
    unsigned char *p;
    int w, h, _bpp;
    p = stbi_load(path.string().c_str(), &w, &h, &_bpp, 4);
    if (p == nullptr) {
        cache_orig_[path] = std::nullopt;
        return cache_orig_.at(path);
    }
    std::vector<unsigned char> data;
    data.resize(w * h * 4);
    for (int i = 0; i < w * h; i++) {
        // alpha: 0ならrgbも0にしないとalpha-blendで悪さをする
        if (p[4 * i + 3] == 0) {
            data[4 * i + 0] = 0;
            data[4 * i + 1] = 0;
            data[4 * i + 2] = 0;
            data[4 * i + 3] = 0;
        }
        else {
            data[4 * i + 0] = p[4 * i + 0];
            data[4 * i + 1] = p[4 * i + 1];
            data[4 * i + 2] = p[4 * i + 2];
            data[4 * i + 3] = p[4 * i + 3];
        }
    }
    stbi_image_free(p);
    if (!use_self_alpha_) {
        auto pna_filename = path.parent_path() / path.stem();
        unsigned char *pna;
        int w_pna, h_pna, _bpp_pna;
        pna = stbi_load(pna_filename.string().c_str(), &w_pna, &h_pna, &_bpp_pna, 4);
        if (pna != nullptr && w == w_pna && h == h_pna) {
            for (int i = 0; i < w * h; i++) {
                int alpha = pna[4 * i];
                data[4 * i + 3] = alpha;
            }
        }
        else {
            int r = data[0 + 0];
            int g = data[0 + 1];
            int b = data[0 + 2];
            for (int i = 0; i < w * h; i++) {
                if (data[4 * i + 0] == r && data[4 * i + 1] == g && data[4 * i + 2] == b) {
                    data[4 * i + 0] = 0;
                    data[4 * i + 1] = 0;
                    data[4 * i + 2] = 0;
                    data[4 * i + 3] = 0;
                }
            }
        }
        if (pna != nullptr) {
            stbi_image_free(pna);
        }
    }
    cache_orig_[path] = {data, w, h, true};
    return cache_orig_.at(path);
}

const std::optional<ImageInfo> &ImageCache::get(const std::filesystem::path &path) {
    if (cache_.contains(path)) {
        return cache_.at(path);
    }
    const auto &info = getOriginal(path);
    if (info == std::nullopt || scale_ == 100) {
        cache_[path] = info;
        return cache_.at(path);
    }
    int w = std::round(info->width() * scale_ / 100.0);
    int h = std::round(info->height() * scale_ / 100.0);
    std::vector<unsigned char> resize;
    resize.resize(w * h * 4);
    stbir_resize_uint8(info->get().data(), info->width(), info->height(), 0, resize.data(), w, h, 0, 4);
    if (scale_ <= 100 || !th_) {
        cache_[path] = {resize, w, h, true};
    }
    else {
        cache_[path] = {resize, w, h, false};
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(path);
        }
        cond_.notify_one();
    }

    return cache_.at(path);
}

void ImageCache::clearCache() {
    cache_.clear();
    cache_orig_.clear();
}
