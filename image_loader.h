#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <cmath>

// 简单的BMP图像加载器（支持24位和32位BMP）
class ImageLoader {
public:
    struct RGB {
        uint8_t r, g, b;
        RGB() : r(0), g(0), b(0) {}
        RGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    };
    
    /**
     * 加载图片文件并返回RGB二维数组
     * @param filename 图片文件名（同目录下）
     * @return RGB二维数组，第一维是高度，第二维是宽度
     */
    static std::vector<std::vector<RGB>> loadImageToRGB(const std::string& filename) {
        // 获取文件扩展名
        std::string ext = filename.substr(filename.find_last_of(".") + 1);
        
        // 转换为小写进行比较
        for (auto& c : ext) {
            c = tolower(c);
        }
        
        if (ext == "bmp") {
            return loadBMP(filename);
        } else if (ext == "ppm") {
            return loadPPM(filename);
        } else {
            throw std::runtime_error("Unsupported image format: " + ext + 
                                   ". Only BMP and PPM are supported.");
        }
    }
    
private:
    // BMP文件头结构（14字节）
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t type;          // 文件类型，应为0x4D42 ("BM")
        uint32_t size;          // 文件大小
        uint16_t reserved1;     // 保留字段1
        uint16_t reserved2;     // 保留字段2
        uint32_t offset;        // 像素数据偏移量
    };
    
    // BMP信息头结构（40字节）
    struct BMPInfoHeader {
        uint32_t size;          // 信息头大小
        int32_t width;          // 图像宽度
        int32_t height;         // 图像高度
        uint16_t planes;        // 颜色平面数，必须为1
        uint16_t bitCount;      // 每像素位数
        uint32_t compression;   // 压缩方式
        uint32_t imageSize;     // 图像数据大小
        int32_t xPixelsPerMeter;// 水平分辨率
        int32_t yPixelsPerMeter;// 垂直分辨率
        uint32_t colorsUsed;    // 使用的颜色数
        uint32_t colorsImportant;// 重要颜色数
    };
    #pragma pack(pop)
    
    /**
     * 加载BMP格式图片
     */
    static std::vector<std::vector<RGB>> loadBMP(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        // 读取文件头
        BMPFileHeader fileHeader;
        file.read(reinterpret_cast<char*>(&fileHeader), sizeof(BMPFileHeader));
        
        // 检查是否为BMP文件
        if (fileHeader.type != 0x4D42) {
            throw std::runtime_error("Not a valid BMP file: " + filename);
        }
        
        // 读取信息头
        BMPInfoHeader infoHeader;
        file.read(reinterpret_cast<char*>(&infoHeader), sizeof(BMPInfoHeader));
        
        // 检查图像格式
        if (infoHeader.bitCount != 24 && infoHeader.bitCount != 32) {
            throw std::runtime_error("Only 24-bit and 32-bit BMP are supported");
        }
        
        if (infoHeader.compression != 0) {
            throw std::runtime_error("Compressed BMP is not supported");
        }
        
        int width = infoHeader.width;
        int height = std::abs(infoHeader.height); // 高度可能为负数（从上到下）
        bool topDown = (infoHeader.height < 0);   // 判断图像方向
        
        // 计算每行字节数（4字节对齐）
        int bytesPerPixel = infoHeader.bitCount / 8;
        int rowStride = (width * bytesPerPixel + 3) & ~3;
        
        // 移动到像素数据位置
        file.seekg(fileHeader.offset, std::ios::beg);
        
        // 读取像素数据
        std::vector<uint8_t> pixelData(rowStride * height);
        file.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
        
        if (!file) {
            throw std::runtime_error("Failed to read pixel data");
        }
        
        // 创建RGB二维数组
        std::vector<std::vector<RGB>> result(height, std::vector<RGB>(width));
        
        // 转换像素数据
        for (int y = 0; y < height; y++) {
            int sourceY = topDown ? y : (height - 1 - y);
            
            for (int x = 0; x < width; x++) {
                int pixelOffset = sourceY * rowStride + x * bytesPerPixel;
                
                // BMP格式存储顺序为BGR
                uint8_t b = pixelData[pixelOffset];
                uint8_t g = pixelData[pixelOffset + 1];
                uint8_t r = pixelData[pixelOffset + 2];
                
                result[y][x] = RGB(r, g, b);
            }
        }
        
        return result;
    }
    
    /**
     * 加载PPM格式图片（P3或P6格式）
     */
    static std::vector<std::vector<RGB>> loadPPM(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        // 读取魔数
        std::string magic;
        file >> magic;
        
        if (magic != "P3" && magic != "P6") {
            throw std::runtime_error("Only P3 and P6 PPM formats are supported");
        }
        
        // 跳过注释
        int c = file.peek();
        while (c == '#') {
            std::string comment;
            std::getline(file, comment);
            c = file.peek();
        }
        
        // 读取宽度、高度和最大值
        int width, height, maxValue;
        file >> width >> height >> maxValue;
        
        if (maxValue > 255) {
            throw std::runtime_error("Only 8-bit PPM images are supported");
        }
        
        // 创建RGB二维数组
        std::vector<std::vector<RGB>> result(height, std::vector<RGB>(width));
        
        if (magic == "P6") {
            // 二进制PPM格式
            file.get(); // 跳过空白字符后的换行符
            
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    uint8_t r = file.get();
                    uint8_t g = file.get();
                    uint8_t b = file.get();
                    
                    if (!file) {
                        throw std::runtime_error("Unexpected end of file");
                    }
                    
                    result[y][x] = RGB(r, g, b);
                }
            }
        } else {
            // 文本PPM格式
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int r, g, b;
                    file >> r >> g >> b;
                    result[y][x] = RGB(static_cast<uint8_t>(r), 
                                      static_cast<uint8_t>(g), 
                                      static_cast<uint8_t>(b));
                }
            }
        }
        
        return result;
    }
};

#endif // IMAGE_LOADER_H