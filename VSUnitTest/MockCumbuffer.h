#pragma once
#include "../CumBuffer.h"

#ifdef _WIN32
#define CACHE_ALIGN __declspec(align(64))  
#endif

#ifdef _WIN32
class CACHE_ALIGN  MockCumbuffer :public CumBuffer::CumBuffer
#endif
{
public:
	void SetBufferLen(int nLen)
	{
		m_BufferLen = nLen;
	}

	void SeCurHead(int nLen)
	{
		m_CurHead = nLen;
	}
	void SetCurTail(int nLen)
	{
		m_CurTail = nLen;
	}

	void SetCumlatedLen(int nLen)
	{
		m_CumulatedLen = nLen;
	}
};