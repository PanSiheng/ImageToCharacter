# ImageToCharacter

将图片转换为字符画的 C++ 程序。

## 功能特点

- 支持 **BMP** 和 **PPM** 格式图片
- 三种输出模式：
  - 灰度 ASCII 字符
  - 灰度中文字符
  - 彩色 emoji 方块
- 可选输出大小（小/大）
- 支持保存结果到文件

## 编译

需要支持 C++17 的编译器：

```bash
g++ -std=c++17 main.cpp -o ImageToCharacter
```

## 使用方法

1. 运行程序：
   ```bash
   ./ImageToCharacter
   ```

2. 按提示输入：
   - 图片文件名（需与程序在同一目录）
   - 输出大小（1=小 50字符宽，2=大 100字符宽）
   - 显示模式（0=ASCII / 1=中文 / 2=彩色）
   - 是否保存到文件

## 示例

```
Enter the filename of the image file: test.bmp
Big or small?(1->small,2->big): 1
Display mode:
  0 -> Grayscale ASCII
  1 -> Grayscale Chinese
  2 -> Color blocks
Choose mode: 0
Save to file? (0->No, 1->Yes): 1
Enter output filename (e.g., output.txt): output.txt
```

## 输出效果

- **ASCII 模式**：使用 `@%#*+=-:. ` 等字符表示灰度
- **中文模式**：使用 `靐翻弛进吃王土人丶` 等汉字表示灰度
- **彩色模式**：使用 🟥🟧🟨🟩🟦🟪🟫⬛⬜ 等 emoji 方块匹配颜色

## 技术细节

- 使用双线性插值进行图片缩放
- 支持 24 位和 32 位 BMP 格式
- 支持 P3 和 P6 PPM 格式
- 输出文件采用 UTF-8 编码

## 许可证

MIT License
