#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <locale>
#include <codecvt>
#include "image_loader.h"

// 设置源文件编码为UTF-8
#pragma execution_character_set("utf-8")

int WIDTH = 50;
bool useChinese = false;
bool useColor = false;

// 汉字字符映射表（从深色到浅色）- 直接使用中文字符
std::vector<std::string> chineseChars = {
    "靐", "翻", "弛", "进", "吃", "百", "王", "土", "人", "丶"
};

// ASCII字符映射表（从深色到浅色）
std::string asciiChars = "@%#*+=-:. ";

// 彩色方块映射表 - 直接使用emoji字符
struct ColorBlock {
    std::string symbol;
    int r, g, b;
};

std::vector<ColorBlock> colorBlocks = {
    {"🟥", 255, 0, 0},      // 红色方块
    {"🟧", 255, 165, 0},    // 橙色方块
    {"🟨", 255, 255, 0},    // 黄色方块
    {"🟩", 0, 255, 0},      // 绿色方块
    {"🟦", 0, 0, 255},      // 蓝色方块
    {"🟪", 128, 0, 128},    // 紫色方块
    {"🟫", 139, 69, 19},    // 棕色方块
    {"⬛", 0, 0, 0},        // 黑色大方块
    {"⬜", 255, 255, 255}    // 白色大方块
};

// 计算颜色距离（欧几里得距离）
double colorDistance(int r1, int g1, int b1, int r2, int g2, int b2) {
    return std::sqrt(std::pow(r1 - r2, 2) + std::pow(g1 - g2, 2) + std::pow(b1 - b2, 2));
}

// 根据RGB值找到最匹配的彩色方块
std::string getColorBlock(int r, int g, int b) {
    int bestIndex = 0;
    double minDistance = colorDistance(r, g, b, colorBlocks[0].r, colorBlocks[0].g, colorBlocks[0].b);
    
    for (int i = 1; i < colorBlocks.size(); i++) {
        double distance = colorDistance(r, g, b, colorBlocks[i].r, colorBlocks[i].g, colorBlocks[i].b);
        if (distance < minDistance) {
            minDistance = distance;
            bestIndex = i;
        }
    }
    
    return colorBlocks[bestIndex].symbol;
}

// 将灰度值映射到字符
std::string getCharForGray(int gray, bool useChinese) {
    if (useChinese) {
        int index = gray * chineseChars.size() / 256;
        if (index >= chineseChars.size()) index = chineseChars.size() - 1;
        return chineseChars[index];
    } else {
        int index = gray * asciiChars.size() / 256;
        if (index >= asciiChars.size()) index = asciiChars.size() - 1;
        return std::string(1, asciiChars[index]);
    }
}

// 生成艺术字
std::string generateArt(const std::vector<std::vector<uint8_t>>& grey, 
                        const std::vector<std::vector<std::tuple<int,int,int>>>& colorData,
                        bool useColor, bool useChinese, 
                        int targetHeight, int targetWidth) {
    std::string result;
    
    for (int i = 0; i < targetHeight; i++) {
        for (int j = 0; j < targetWidth; j++) {
            if (useColor) {
                auto [r, g, b] = colorData[i][j];
                result += getColorBlock(r, g, b);
            } else {
                result += getCharForGray(grey[i][j], useChinese);
            }
        }
        result += "\n";
    }
    
    return result;
}

int main(){
    // 设置locale以支持UTF-8
    // std::ios::sync_with_stdio(false);
    // std::locale::global(std::locale(""));
    // std::wcout.imbue(std::locale(""));
    
    std::string imageName;
    std::cout << "Enter the filename of the image file: ";
    std::getline(std::cin, imageName);
    
    std::cout << "Big or small?(1->small,2->big): ";
    std::cin >> WIDTH;
    if(WIDTH==1) WIDTH=50;
    else WIDTH=100;
    
    std::cout << "Display mode:\n";
    std::cout << "  0 -> Grayscale ASCII\n";
    std::cout << "  1 -> Grayscale Chinese\n";
    std::cout << "  2 -> Color blocks\n";
    std::cout << "Choose mode: ";
    int mode;
    std::cin >> mode;
    
    useChinese = (mode == 1);
    useColor = (mode == 2);
    
    std::cout << "Save to file? (0->No, 1->Yes): ";
    int saveToFile;
    std::cin >> saveToFile;
    
    std::string outputFilename;
    if (saveToFile) {
        std::cout << "Enter output filename (e.g., output.txt): ";
        std::cin >> outputFilename;
    }
    
    // 设置控制台为UTF-8编码
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    
    // 加载原始图片
    auto image = ImageLoader::loadImageToRGB(imageName);
    
    int originalHeight = image.size();
    int originalWidth = image[0].size();
    
    int targetHeight, targetWidth;
    
    if (useChinese || useColor) {
        // 汉字和彩色方块都是正方形，不需要拉伸
        targetWidth = WIDTH;
        targetHeight = static_cast<int>(static_cast<double>(originalHeight) / originalWidth * targetWidth);
        
        if (targetHeight < 1) targetHeight = 1;
    } else {
        // 原有的ASCII逻辑（考虑字符拉伸）
        targetHeight = originalHeight/(originalWidth/(WIDTH/2));
        targetWidth = WIDTH;
    }
    
    // 计算缩放比例
    double scaleY = static_cast<double>(originalHeight) / targetHeight;
    double scaleX = static_cast<double>(originalWidth) / targetWidth;
    
    // 创建缩放后的灰度图像和彩色数据
    std::vector<std::vector<uint8_t>> grey(targetHeight, std::vector<uint8_t>(targetWidth));
    std::vector<std::vector<std::tuple<int,int,int>>> colorData;
    
    if (useColor) {
        colorData.resize(targetHeight, std::vector<std::tuple<int,int,int>>(targetWidth));
    }
    
    // 使用双线性插值进行缩放
    for (int i = 0; i < targetHeight; i++) {
        for (int j = 0; j < targetWidth; j++) {
            // 计算对应原始图像中的坐标
            double srcY = i * scaleY;
            double srcX = j * scaleX;
            
            // 获取四个相邻像素的整数坐标
            int y1 = static_cast<int>(srcY);
            int y2 = std::min(y1 + 1, originalHeight - 1);
            int x1 = static_cast<int>(srcX);
            int x2 = std::min(x1 + 1, originalWidth - 1);
            
            // 计算插值权重
            double dy = srcY - y1;
            double dx = srcX - x1;
            
            // 获取四个相邻像素的RGB值
            auto rgb11 = image[y1][x1];
            auto rgb12 = image[y1][x2];
            auto rgb21 = image[y2][x1];
            auto rgb22 = image[y2][x2];
            
            if (useColor) {
                // 彩色模式：对RGB三个通道分别进行双线性插值
                double r = (rgb11.r * (1-dx) + rgb12.r * dx) * (1-dy) + 
                          (rgb21.r * (1-dx) + rgb22.r * dx) * dy;
                double g = (rgb11.g * (1-dx) + rgb12.g * dx) * (1-dy) + 
                          (rgb21.g * (1-dx) + rgb22.g * dx) * dy;
                double b = (rgb11.b * (1-dx) + rgb12.b * dx) * (1-dy) + 
                          (rgb21.b * (1-dx) + rgb22.b * dx) * dy;
                
                colorData[i][j] = {static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)};
            } else {
                // 灰度模式：转换为灰度值
                double g11 = rgb11.r * 0.299 + rgb11.g * 0.587 + rgb11.b * 0.114;
                double g12 = rgb12.r * 0.299 + rgb12.g * 0.587 + rgb12.b * 0.114;
                double g21 = rgb21.r * 0.299 + rgb21.g * 0.587 + rgb21.b * 0.114;
                double g22 = rgb22.r * 0.299 + rgb22.g * 0.587 + rgb22.b * 0.114;
                
                double top = g11 * (1 - dx) + g12 * dx;
                double bottom = g21 * (1 - dx) + g22 * dx;
                double grayValue = top * (1 - dy) + bottom * dy;
                
                grey[i][j] = static_cast<uint8_t>(grayValue);
            }
        }
    }
    
    // 生成艺术字
    std::string art;
    if (useColor) {
        art = generateArt(grey, colorData, true, false, targetHeight, targetWidth);
    } else {
        art = generateArt(grey, colorData, false, useChinese, targetHeight, targetWidth);
    }
    
    // 输出到控制台
    std::cout << "\n" << art << std::endl;
    std::cout << "Image size: " << targetWidth << " x " << targetHeight << std::endl;
    
    // 保存到文件
    if (saveToFile) {
        std::ofstream outFile(outputFilename, std::ios::out | std::ios::binary);
        if (outFile.is_open()) {
            // 写入UTF-8 BOM
            unsigned char bom[] = {0xEF, 0xBB, 0xBF};
            outFile.write(reinterpret_cast<char*>(bom), sizeof(bom));
            
            // 写入艺术字和图片信息
            outFile << art;
            outFile << "\nImage size: " << targetWidth << " x " << targetHeight << "\n";
            outFile << "Mode: ";
            if (useColor) outFile << "Color blocks";
            else if (useChinese) outFile << "Grayscale Chinese";
            else outFile << "Grayscale ASCII";
            outFile << "\n";
            
            outFile.close();
            std::cout << "Art saved to: " << outputFilename << std::endl;
        } else {
            std::cout << "Error: Cannot open file " << outputFilename << " for writing!" << std::endl;
        }
    }
    
    return 0;
}
