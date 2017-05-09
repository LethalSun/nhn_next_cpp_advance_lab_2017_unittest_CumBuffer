#pragma once
#ifndef __CUMBUFFER_HPP__
#define __CUMBUFFER_HPP__
/****************************************************************************
 Copyright (c) 2016, ko jung hyun
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/
// https://github.com/jeremyko/CumBuffer

// NO THREAD SAFETY HERE
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <cstdlib>
#include <memory>

namespace CumBuffer
{
	//#define CUMBUFFER_DEBUG


	const int DEFAULT_BUFFER_LEN = 1024 * 4;
	const int CACHE_LINE_SIZE = 64;

	enum class OP_RESULT : int
	{
		OP_RSLT_OK = 0,
		OP_RSLT_NO_DATA,
		OP_RSLT_BUFFER_FULL,
		OP_RSLT_ALLOC_FAILED,
		OP_RSLT_INVALID_LEN,
		OP_RSLT_HEAD_TAIL,
		OP_RSLT_TAIL_HEAD_BUT_READ_ONEBLOCK,
		OP_RSLT_TAIL_HEAD_READ_TWOBLOCK
	};

#ifdef _WIN32
#define CACHE_ALIGN __declspec(align(CACHE_LINE_SIZE))  
#endif

#if defined __APPLE__ || defined __linux__ 
#define CACHE_ALIGN __attribute__ ((aligned(cumbuffer_defines::CACHE_LINE_SIZE))) 
#endif

	///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
	class CACHE_ALIGN CumBuffer
#endif
#if defined __APPLE__ || defined __linux__ 
		class CumBuffer
#endif
	{
	public:

		CumBuffer() = default;

		virtual ~CumBuffer() = default;

		//복사생성자 와 대입연산자 제거
		CumBuffer(const CumBuffer&) = delete;
		CumBuffer& operator = (const CumBuffer &) = delete;

		//TODO:포인터 할당은 스마트 포인터로 바꾸기
		OP_RESULT    Init(int nMaxBufferLen = DEFAULT_BUFFER_LEN);
		
		OP_RESULT    Append(const size_t nLen, const char* pData);

		OP_RESULT CheckAppendDataBufferLength(size_t appendLength);
		

		OP_RESULT CheckAppendDataSpaceAfterRotation(const char* pAppendData, const size_t appendLength);


		OP_RESULT AppendDataWhenRotate(const char* pAppendData, const size_t appendLength);

		//------------------------------------------------------------------------
		// 데이터는 가져오지만 head를 이동 시키지 않는다
		OP_RESULT    PeekData(size_t nLen, char* pDataOut);
		
		// 데이터는 가져오지 않고 head만 이동 시킨다
		OP_RESULT    ConsumeData(size_t nLen);

		//------------------------------------------------------------------------

		OP_RESULT CheckGetDataAvailable(size_t nLen);

		bool IsDataNotRotated(size_t nLen);

		bool IsDataRotatedAndReadOneBlock(size_t nLen);

		bool IsDataRotatedAndReadTwoBlock(size_t nLen);
/*
		OP_RESULT    GetData(size_t  nLen,
			char*   pDataOut,
			bool    bPeek = false,
			bool    bMoveHeaderOnly = false)
		{
			if (bPeek && bMoveHeaderOnly)
			{
				std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid usage" << "\n";
				m_strErrMsg = "invalid usage";
				return OP_RESULT::OP_RSLT_INVALID_USAGE;
			}

#ifdef CUMBUFFER_DEBUG
			DebugPos(__LINE__);
#endif

			OP_RESULT nRslt = ValidateBuffer(nLen);
			if (OP_RESULT::OP_RSLT_OK != nRslt)
			{
				std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid buffer :" << m_strErrMsg << "\n";
				return nRslt;
			}

			if (m_CurTail > m_CurHead)
			{
				//일반적인 경우
				//TODO: 중복 확인 아닐까?
				if (m_CurTail < m_CurHead + nLen)
				{
					std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid length :" << nLen << "\n";
					m_strErrMsg = "invalid length";
					return OP_RESULT::OP_RSLT_INVALID_LEN;
				}
				else
				{
					if (!bMoveHeaderOnly)
					{
						memcpy(pDataOut, m_pBuffer + m_CurHead, nLen);
					}
					if (!bPeek)
					{
						m_CurHead += nLen;
					}
				}
			}
			else// if(nCurTail_ <= nCurHead_)
			{
				if (m_BufferLen < m_CurHead + nLen)
				{
					size_t nFirstBlockLen = m_BufferLen - m_CurHead;
					size_t nSecondBlockLen = nLen - nFirstBlockLen;
#ifdef CUMBUFFER_DEBUG
					std::cout << "[" << __func__ << "-" << __LINE__ << "] nFirstBlockLen=" << nFirstBlockLen
						<< "/nSecondBlockLen=" << nSecondBlockLen << "\n";
#endif

					if (m_CurTail > 0 &&
						m_CurTail >= nSecondBlockLen)
					{
						if (!bMoveHeaderOnly)
						{
							memcpy(pDataOut, m_pBuffer + m_CurHead, nFirstBlockLen);
							memcpy(pDataOut + nFirstBlockLen, m_pBuffer, nSecondBlockLen);
						}

						if (!bPeek)
						{
							m_CurHead = nSecondBlockLen;
						}
					}
					else
					{
						std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid length :" << nLen
							<< " / nFirstBlockLen =" << nFirstBlockLen << "/nSecondBlockLen=" << nSecondBlockLen << "\n";
						m_strErrMsg = "invalid length";
						return OP_RESULT::OP_RSLT_INVALID_LEN;
					}
				}
				else
				{
					if (!bMoveHeaderOnly)
					{
						memcpy(pDataOut, m_pBuffer + m_CurHead, nLen);
					}

					if (!bPeek)
					{
						m_CurHead += nLen;
					}
				}
			}

			if (!bPeek)
			{
				m_CumulatedLen -= nLen;
			}

#ifdef CUMBUFFER_DEBUG
			std::cout << "[" << __func__ << "-" << __LINE__ << "] out data [" << pDataOut << "]\n";
			DebugPos(__LINE__);
#endif

			return OP_RESULT::OP_RSLT_OK;
		}
*/

		OP_RESULT ValidateBuffer(size_t nLen);
		
		size_t GetCumulatedLen() 
		{
			return m_CumulatedLen;
		}

		size_t GetCapacity() 
		{
			return m_BufferLen;
		}

		size_t GetTotalFreeSpace() 
		{
			return m_BufferLen - m_CumulatedLen;
		}

		uint64_t GetCurHeadPos() 
		{
			return m_CurHead;
		}

		uint64_t GetCurTailPos() 
		{
			return m_CurTail;
		}
/* 임의로 직접 버퍼에 쓰는 것은 막는다. 무의미하고 위험하다. 이렇게 사용한다는건 구조를 알고 있다는 건데
//그렇다면 Append와 ConsumData 로 원하는 위치에 쓸수 있고 정말 임의 의 위치에 쓰고 싶으면 헤드와 테일만으로
//관리가 불가능하다. 관리할 필요가 없다면 즉 기존 데이터에 관심이 없다면 리셋을 하고 쓰면 된다.
		uint64_t GetLinearFreeSpace() //for direct buffer write
		{
			//current maximun linear buffer size

			if (m_CurTail == m_BufferLen) //nCurTail_ is at last position
			{
				return m_BufferLen - m_CumulatedLen;
			}
			else if (m_CurHead < m_CurTail)
			{
				return m_BufferLen - m_CurTail;
			}
			else if (m_CurHead > m_CurTail)
			{
				return m_CurHead - m_CurTail;
			}
			else
			{
				return m_BufferLen - m_CurTail;
			}
		}

		char* GetLinearAppendPtr() //for direct buffer write
		{
			if (m_CurTail == m_BufferLen) //nCurTail_ is at last position
			{
				if (m_BufferLen != m_CumulatedLen) //and buffer has free space
				{
					//-> append at 0  
					//nCurTail_ -> 버퍼 마지막 위치하고, 버퍼에 공간이 존재. -> 처음에 저장
					//XXX dangerous XXX 
					//this is not a simple get function, nCurTail_ changes !!
					m_CurTail = 0;
				}
			}

			return (m_pBuffer + m_CurTail);
		}

		void IncreaseData(size_t nLen)
		{
			m_CurTail += nLen;
			m_CumulatedLen += nLen;
		}
*/
		void    DebugPos(int nLine)
		{
			std::cout << "line=" << nLine << "/ nCurHead_=" << m_CurHead << "/ nCurTail_= " << m_CurTail
				<< " / nBufferLen_=" << m_BufferLen
				<< " / nCumulatedLen_=" << m_CumulatedLen
				<< "\n";
		}

		void Reset()
		{
			m_CumulatedLen = 0;
			m_CurHead = 0;
			m_CurTail = 0;
		}

		std::string GetErrMsg() { return m_strErrMsg; }


	protected:
		std::string m_strErrMsg;

		std::unique_ptr<char[]>m_pBuffer{ nullptr };
		size_t      m_BufferLen = 0;
		size_t      m_CumulatedLen = 0; // 저장된 데이터 길이

		uint64_t    m_CurHead = 0; // 가장 최신 읽기의 마지막 위치
		uint64_t    m_CurTail = 0; // 가장 최신 쓰기의 마지막 위치
	}
#if defined __APPLE__ || defined __linux__ 
	CACHE_ALIGN
#endif
		;

#endif
}






