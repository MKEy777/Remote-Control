#pragma once
class Tool
{
public:
    static void Dump(BYTE* pData, size_t nSize) {//调试输出十六进制数据
        std::string out;
        for (size_t i = 0;i < nSize;i++) {
            char buf[8] = "";
            sprintf_s(buf, sizeof(buf), "%02X ", pData[i]);//把一个字节格式化成可读的十六进制字符串
            out += buf;
        }
        out += '\n';
        OutputDebugStringA(out.c_str());//将字符串输出到 Visual Studio 的“输出”窗口（Debug 模式下可见）
    }
};

