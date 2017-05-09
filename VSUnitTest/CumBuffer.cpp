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
		//���� ���̰� ���� ��ü�� ũ�� ���� ũ�� ����,���۰� ���� ���־ ����
		auto opRet = CheckAppendDataBufferLength(nLen);

		if (opRet != OP_RESULT::OP_RSLT_OK)
		{
			return opRet;
		}

		if (m_CurTail < m_CurHead)
		{
			//tail �� ���� ���� ���� ���
			//������� ���� �ϰ� ����
			opRet = CheckAppendDataSpaceAfterRotation(pData, nLen);
			return opRet;
		}
		else
		{

			if (m_BufferLen < m_CurTail + nLen)  //tail ����, ���� ���۷� ���ڶ�� ���
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
		//�̻�Ȳ���� m_CurHead �� m_CurTail ���� ũ�ٴ� �������� ���� ������ ����
		const auto space = m_CurHead - m_CurTail;

		//���� ������ �������� ū ���̸� �����ϴ°��� �Ұ���
		if (space < appendLength)
		{
			std::cerr << "[" << __func__ << "-" << __LINE__ << "] buffer full" << "\n";
			m_strErrMsg = "buffer full";
			return OP_RESULT::OP_RSLT_BUFFER_FULL;
		}

		//���� ���� �ϴٸ� ����
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
		//������ ó������ ���ư����� �ʿ��� ���� ������ ����Ѵ�.
		const auto spaceRequiredAfterRotation = appendLength - (m_BufferLen - m_CurTail);

		//2�� ����� �� ������ �ִ� ��� ���⼭ ������ 0�ΰ��� ���۰� ó�� ����� �������̴�,
		if (m_CurTail > 0 && spaceRequiredAfterRotation <= m_CurHead)
		{
#ifdef CUMBUFFER_DEBUG
			DebugPos(__LINE__);
#endif
			//�ι� ������ �� ������ ����Ѵ�.
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
		//PeekData ����ؼ� ó���� data length ��ŭ ���۳� nCurHead_ �� �̵�.
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
		//������� �о������ �ϸ� �ȵ�
		if (m_CumulatedLen == 0)
		{
			m_strErrMsg = "no data";
			return OP_RESULT::OP_RSLT_NO_DATA;
		}
		//����Ǿ��ִ� �ͺ��� ���� �о������ �ϸ� �ȵ�
		else if (m_CumulatedLen < nLen)
		{
			m_strErrMsg = "invalid length";
			return OP_RESULT::OP_RSLT_INVALID_LEN;
		}

		return OP_RESULT::OP_RSLT_OK;
	}

}