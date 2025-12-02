#pragma once
#include "pch.h"
#include "framework.h"

class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), strData(""), sSum(0) {}
	CPacket& operator=(const CPacket& packet) {
		if (this != &packet) {
			sHead = packet.sHead;
			nLength = packet.nLength;
			sCmd = packet.sCmd;
			strData = packet.strData;
			sSum = packet.sSum;
		}
		return *this;
	}
	//数据包封包构造函数，用于把数据封装成数据包
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)&strData[0], pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	//数据包解包，将包中的数据分配到成员变量中
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;

		// ① 查找包头 0xFEFF
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // 判断包头标识（两个字节）
			{
				sHead = *(WORD*)(pData + i);   // 保存包头
				i += 2;                        // 跳过包头字节
				break;
			}
		}
		// 长度+命令+校验和
		if (i + 4 + 2 + 2 > nSize)
		{
			nSize = 0;     // 表示包不完整
			return;
		}

		// ② 读取包总长度（4字节）
		nLength = *(DWORD*)(pData + i); // 从当前偏移位置取 4 字节长度
		i += 4;
		if (nLength + i > nSize)        // 判断数据是否接收完整
		{
			nSize = 0;                  // 数据不完整，继续接收
			return;                     // 解析失败
		}

		// ③ 读取命令字段（2字节）
		sCmd = *(WORD*)(pData + i);     // 取命令字
		i += 2;

		// ④ 读取包体数据
		if (nLength > 4) // 有数据区
		{
			strData.resize(nLength - 2 - 2);                        // 设置 string 大小
			// void * memcpy(void *_Dst, const void *_Src, size_t _Size) 
			memcpy(&strData[0], pData + i, nLength - 4);
			i += nLength - 4;                                        // 移动偏移量
		}

		// ⑤ 读取校验码并计算校验
		sSum = *(WORD*)(pData + i); // 从数据末尾读取 2 字节校验码
		i += 2;

		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) // 对数据部分计算校验和
			sum += BYTE(strData[j]) & 0xFF;         // 仅取低 8 位累加

		if (sum == sSum)   // 校验成功
		{
			nSize = i;    // 返回包总长度（包头 + 长度字段 + 命令 + 数据 + 校验）
			return;
		}

		// 校验失败
		nSize = 0;
	}

	~CPacket() {

	}

	int Size() {//包数据的大小
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)&strOut[0];
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	WORD sHead;         //包头
	DWORD nLength;      //包长度,从命令字段到校验码的总长度
	WORD sCmd; //命令字
	std::string strData;//数据
	WORD sSum;          //校验和
	std::string strOut;//整个包的数据
};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、中键、右键
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;
typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录 0 否 1 是
	BOOL HasNext;//是否还有后续 0 没有 1 有
	char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;