#include "stdafx.h"
#include "../CumBuffer.h"

namespace CumBuffer
{

	OP_RESULT CumBuffer::Init(int nMaxBufferLen)
	{
		m_BufferLen = nMaxBufferLen;
		m_pBuffer = std::move(std::unique_ptr<char[]>(new char[m_BufferLen]));

		if (m_pBuffer == nullptr)
		{
			std::cerr << "[" << __func__ << "-" << __LINE__ << "] alloc failed.\n";
			m_strErrMsg = "alloc failed.";
			return OP_RESULT::OP_RSLT_ALLOC_FAILED;
		}

		return OP_RESULT::OP_RSLT_OK;
	}


	OP_RESULT CumBuffer::Append(const size_t nLen, const char * pData)
	{
#ifdef CUMBUFFER_DEBUG
		std::cout << "[" << __func__ << "-" << __LINE__ << "] nLen=" << nLen << "[" << pData << "]\n";
		DebugPos(__LINE__);
#endif
		//저장 길이가 버퍼 전체의 크기 보다 크면 에러,버퍼가 가득 차있어도 에러
		auto opRet = CheckAppendDataBufferLength(nLen);

		if (opRet != OP_RESULT::OP_RSLT_OK)
		{
			return opRet;
		}

		if (m_CurTail < m_CurHead)
		{
			//tail 이 버퍼 끝을 지난 경우
			//적용까지 같이 하고 있음
			opRet = CheckAppendDataSpaceAfterRotation(pData, nLen);
			return opRet;
		}
		else
		{

			if (m_BufferLen < m_CurTail + nLen)  //tail 이후, 남은 버퍼로 모자라는 경우
			{
				opRet = AppendDataWhenRotate(pData, nLen);

				if (opRet != OP_RESULT::OP_RSLT_OK) {
					return opRet;
				}
			}
			else
			{
				//most general case
				memcpy(m_pBuffer.get() + m_CurTail, pData, nLen);
				m_CurTail += nLen;
				m_CumulatedLen += nLen;

#ifdef CUMBUFFER_DEBUG
				DebugPos(__LINE__);
#endif
				return OP_RESULT::OP_RSLT_OK;
			}
		}

		return OP_RESULT::OP_RSLT_OK;
	}


	OP_RESULT CumBuffer::CheckAppendDataBufferLength(size_t appendLength)
	{
		if (m_BufferLen < appendLength)
		{
			std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid len :" << appendLength << "\n";
			m_strErrMsg = "invalid length";
			return OP_RESULT::OP_RSLT_INVALID_LEN;
		}
		else if (m_BufferLen == m_CumulatedLen)
		{
			std::cerr << "[" << __func__ << "-" << __LINE__ << "] buffer full" << "\n";
			m_strErrMsg = "buffer full";
			return OP_RESULT::OP_RSLT_BUFFER_FULL;
		}

		return OP_RESULT::OP_RSLT_OK;
	}
	
	
	OP_RESULT CumBuffer::CheckAppendDataSpaceAfterRotation(const char * pAppendData, const size_t appendLength)
	{
		//이상황에서 m_CurHead 가 m_CurTail 보다 크다는 가정에서 저장 가능한 공간
		const auto space = m_CurHead - m_CurTail;

		//저장 가능한 공간보다 큰 길이를 저장하는것은 불가능
		if (space < appendLength)
		{
			std::cerr << "[" << __func__ << "-" << __LINE__ << "] buffer full" << "\n";
			m_strErrMsg = "buffer full";
			return OP_RESULT::OP_RSLT_BUFFER_FULL;
		}

		//저장 가능 하다면 적용
		memcpy(m_pBuffer.get() + m_CurTail, pAppendData, appendLength);
		m_CurTail += appendLength;
		m_CumulatedLen += appendLength;

#ifdef CUMBUFFER_DEBUG
		DebugPos(__LINE__);
#endif
		return OP_RESULT::OP_RSLT_OK;
	}


	OP_RESULT CumBuffer::AppendDataWhenRotate(const char * pAppendData, const size_t appendLength)
	{
		//테일이 처음으로 돌아간다음 필요한 저장 공간을 계산한다.
		const auto spaceRequiredAfterRotation = appendLength - (m_BufferLen - m_CurTail);

		//2번 나누어서 들어갈 공간이 있는 경우 여기서 테일이 0인경우는 버퍼가 처음 비어져 있을때이다,
		if (m_CurTail > 0 && spaceRequiredAfterRotation <= m_CurHead)
		{
#ifdef CUMBUFFER_DEBUG
			DebugPos(__LINE__);
#endif
			//두번 나눠서 들어갈 공간을 계산한다.
			auto nFirstBlockLen = static_cast<int>(m_BufferLen - m_CurTail);
			auto nSecondBlockLen = static_cast<int>(appendLength - nFirstBlockLen);

#ifdef CUMBUFFER_DEBUG
			std::cout << "[" << __func__ << "-" << __LINE__ << "] nFirstBlockLen =" << nFirstBlockLen
				<< "/nSecondBlockLen=" << nSecondBlockLen << "\n";
#endif

			if (nFirstBlockLen > 0)
			{
				memcpy(m_pBuffer.get() + m_CurTail, pAppendData, nFirstBlockLen);
			}

			memcpy(m_pBuffer.get(), pAppendData + (nFirstBlockLen), nSecondBlockLen);

			m_CurTail = nSecondBlockLen;
			m_CumulatedLen += appendLength;
#ifdef CUMBUFFER_DEBUG
			DebugPos(__LINE__);
#endif
			return OP_RESULT::OP_RSLT_OK;
		}

		std::cerr << "[" << __func__ << "-" << __LINE__ << "] buffer full" << "\n";
		m_strErrMsg = "buffer full";
		return OP_RESULT::OP_RSLT_BUFFER_FULL;
	}
	OP_RESULT CumBuffer::PeekData(size_t nLen, char * pDataOut)
	{
		auto result = CheckGetDataAvailable(nLen);
		switch (result)
		{
		case OP_RESULT::OP_RSLT_HEAD_TAIL:
		{
			memcpy(pDataOut, m_pBuffer.get() + m_CurHead, nLen);
			break;
		}
		case OP_RESULT::OP_RSLT_TAIL_HEAD_BUT_READ_ONEBLOCK:
		{
			memcpy(pDataOut, m_pBuffer.get() + m_CurHead, nLen);
			break;
		}
		case OP_RESULT::OP_RSLT_TAIL_HEAD_READ_TWOBLOCK:
		{
			size_t nFirstBlockLen = m_BufferLen - m_CurHead;
			size_t nSecondBlockLen = nLen - nFirstBlockLen;
			memcpy(pDataOut, m_pBuffer.get() + m_CurHead, nFirstBlockLen);
			memcpy(pDataOut + nFirstBlockLen, m_pBuffer.get(), nSecondBlockLen);
			break;
		}
		default:
			return result;
		}
#ifdef CUMBUFFER_DEBUG
		std::cout << "[" << __func__ << "-" << __LINE__ << "] out data [" << pDataOut << "]\n";
		DebugPos(__LINE__);
#endif
		return OP_RESULT::OP_RSLT_OK;
	}

	OP_RESULT CumBuffer::ConsumeData(size_t nLen)
	{
		//PeekData 사용해서 처리한 data length 만큼 버퍼내 nCurHead_ 를 이동.
		auto result = CheckGetDataAvailable(nLen);
		switch (result)
		{
		case OP_RESULT::OP_RSLT_HEAD_TAIL:
		{
			m_CurHead += nLen;
			break;
		}
		case OP_RESULT::OP_RSLT_TAIL_HEAD_BUT_READ_ONEBLOCK:
		{
			m_CurHead += nLen;
			break;
		}
		case OP_RESULT::OP_RSLT_TAIL_HEAD_READ_TWOBLOCK:
		{
			size_t nFirstBlockLen = m_BufferLen - m_CurHead;
			size_t nSecondBlockLen = nLen - nFirstBlockLen;
			m_CurHead = nSecondBlockLen;
			break;
		}
		default:
			return result;
		}

		m_CumulatedLen -= nLen;

#ifdef CUMBUFFER_DEBUG
		std::cout << "[" << __func__ << "-" << __LINE__ << "] out data [" << pDataOut << "]\n";
		DebugPos(__LINE__);
#endif
		return OP_RESULT::OP_RSLT_OK;
	}

	OP_RESULT CumBuffer::CheckGetDataAvailable(size_t nLen)
	{

#ifdef CUMBUFFER_DEBUG
		DebugPos(__LINE__);
#endif

		auto nRslt = ValidateBuffer(nLen);
		if (OP_RESULT::OP_RSLT_OK != nRslt)
		{
			std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid buffer :" << m_strErrMsg << "\n";
			return nRslt;
		}

		if (IsDataNotRotated(nLen))
		{
			return OP_RESULT::OP_RSLT_HEAD_TAIL;
		}
		else if (IsDataRotatedAndReadOneBlock(nLen))
		{
			return OP_RESULT::OP_RSLT_TAIL_HEAD_BUT_READ_ONEBLOCK;
		}
		else if (IsDataRotatedAndReadTwoBlock(nLen))
		{
			return OP_RESULT::OP_RSLT_TAIL_HEAD_READ_TWOBLOCK;
		}
		else
		{
			return OP_RESULT::OP_RSLT_INVALID_LEN;
		}

	}

	bool CumBuffer::IsDataNotRotated(size_t nLen)
	{
		if (m_CurTail > m_CurHead)
		{
			if (m_CurTail < m_CurHead + nLen)
			{
				std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid length :" << nLen << "\n";
				m_strErrMsg = "invalid length";
				return false;
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	bool CumBuffer::IsDataRotatedAndReadOneBlock(size_t nLen)
	{
		if (m_CurTail > m_CurHead)
		{
			return false;
		}

		if (m_BufferLen >= m_CurHead + nLen)
		{
			return true;
		}

		return false;
	}

	bool CumBuffer::IsDataRotatedAndReadTwoBlock(size_t nLen)
	{
		if (m_CurTail > m_CurHead)
		{
			return false;
		}

		if (m_BufferLen < m_CurHead + nLen)
		{
			auto nFirstBlockLen = m_BufferLen - m_CurHead;
			auto nSecondBlockLen = nLen - nFirstBlockLen;
#ifdef CUMBUFFER_DEBUG
			std::cout << "[" << __func__ << "-" << __LINE__ << "] nFirstBlockLen=" << nFirstBlockLen
				<< "/nSecondBlockLen=" << nSecondBlockLen << "\n";
#endif

			if (m_CurTail > 0 &&
				m_CurTail >= nSecondBlockLen)
			{
				return true;
			}
			else
			{
				std::cerr << "[" << __func__ << "-" << __LINE__ << "] invalid length :" << nLen
					<< " / nFirstBlockLen =" << nFirstBlockLen << "/nSecondBlockLen=" << nSecondBlockLen << "\n";
				m_strErrMsg = "invalid length";
				return false;
			}
		}

		return false;
	}

	OP_RESULT CumBuffer::ValidateBuffer(size_t nLen)
	{
		//비었을때 읽어오려고 하면 안됨
		if (m_CumulatedLen == 0)
		{
			m_strErrMsg = "no data";
			return OP_RESULT::OP_RSLT_NO_DATA;
		}
		//저장되어있는 것보다 많이 읽어오려고 하면 안됨
		else if (m_CumulatedLen < nLen)
		{
			m_strErrMsg = "invalid length";
			return OP_RESULT::OP_RSLT_INVALID_LEN;
		}

		return OP_RESULT::OP_RSLT_OK;
	}

}