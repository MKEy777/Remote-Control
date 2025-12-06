#pragma once
#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>


class Tool
{
public:
	static void Dump(BYTE* pData, size_t nSize)
	{
		std::string strOut;
		for (size_t i = 0; i < nSize; i++)
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0))strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}
	static int Bytes2Image(CImage& image, const std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str();
		//存入CImage
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//创建一个全局内存块
		if (hMem == NULL) {
			TRACE("GlobalAlloc failed\r\n");
			Sleep(10);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//把全局内存包装成一个 COM 流 (IStream)
		if (hRet == S_OK) {
			ULONG length = 0;//写入流的字节数
			pStream->Write(pData, strBuffer.size(), &length);//把数据写入流
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);//把流的指针移到开头
			// 如果 m_image 里已经有图了，先把它销毁
			if (!image.IsNull()) {
				image.Destroy();
			}
			image.Load(pStream);//从流中加载图像数据
		}
		return hRet;
	}
};

