#include "stdafx.h"
#include "CppUnitTest.h"
#include <random>
#include <chrono>
#include <algorithm>    
#include "../CumBuffer.h"
#include "MockCumbuffer.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const uint64_t UINT64_ZERO = 0;

void GetTwoRandomNum(int& n1,int& n2)
{
	std::random_device rand;

	std::uniform_int_distribution<int> dist(0, 30 * 1024);

	n1 = dist(rand);
	n2 = dist(rand);
}

void GetFourRandomNum(int& n1, int& n2,int& n3)
{
	std::random_device rand;

	std::uniform_int_distribution<int> dist(0, 30 * 1024);

	n1 = dist(rand);
	n2 = dist(rand);
	n3 = dist(rand);
}

void MakeString(std::string& str,int nLen)
{
	for (int i = 0; i < nLen; ++i)
	{
		str += "a";
	}
}

namespace VSUnitTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:

		TEST_METHOD(InitBufferlenValidate_TestNoInput)
		{
			CumBuffer::CumBuffer bufferingDefaltLen;
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == bufferingDefaltLen.Init());
			Assert::AreEqual(bufferingDefaltLen.GetCapacity(), (size_t)CumBuffer::DEFAULT_BUFFER_LEN);
		}

		TEST_METHOD(InitBufferlenValidate_TestInput)
		{
			for (int i = 1; i <= 10; ++i)
			{
				auto buffering = new MockCumbuffer;

				Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering->Init(1024 * i));
				Assert::AreEqual(buffering->GetCapacity(), (size_t)1024 * i);
				delete(buffering);
			}
		}
		
		TEST_METHOD(AppendAndGet_Test)
		{
			char data[100] = { 0, };
			char dataOut[100] = { 0, };

			CumBuffer::CumBuffer buffering;
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.Init()); //default buffer length
			Assert::AreEqual(buffering.GetCurHeadPos(), UINT64_ZERO);
			Assert::AreEqual(buffering.GetCurTailPos(), UINT64_ZERO);
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_NO_DATA == buffering.PeekData(3, dataOut));

			memset(data, 0x00, sizeof(data));
			memcpy(data, (void*)"aaaa", 4);
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.Append(4, data));
			Assert::AreEqual(buffering.GetCurTailPos(), (uint64_t)4);

			memset(data, 0x00, sizeof(data));
			memcpy(data, (void*)"bbbb", 4);
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.Append(4, data));
			Assert::AreEqual(buffering.GetCurTailPos(), (uint64_t)8);
			Assert::AreEqual(buffering.GetCumulatedLen(), (uint64_t)8);

			memset(dataOut, 0x00, sizeof(dataOut));
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN == buffering.PeekData(9, dataOut));
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.PeekData(8, dataOut));
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.ConsumeData(8));
			Assert::AreEqual("aaaabbbb", dataOut);
			Assert::AreEqual(buffering.GetCurHeadPos(), (uint64_t)8);
			Assert::AreEqual(buffering.GetCurTailPos(), (uint64_t)8);
		}

		TEST_METHOD(CheckAppendDataBufferLength_TestInvalidLen)
		{
			for (int i = 1; i < 100; ++i)
			{
				auto rNum1 = 0, rNum2 = 0;

				GetTwoRandomNum(rNum1, rNum2);

				CumBuffer::CumBuffer buffering;

				buffering.Init(rNum1 );

				if (rNum1 < rNum2)
				{
					Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN == buffering.CheckAppendDataBufferLength(rNum2));
				}
				else
				{
					Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.CheckAppendDataBufferLength(rNum2));
				}	
			}
		}

		TEST_METHOD(CheckAppendDataBufferLength_TestBufferFull)
		{
			for (int i = 1; i < 100; ++i)
			{
				auto rNum1 = 0, rNum2 = 0;

				GetTwoRandomNum(rNum1, rNum2);

				MockCumbuffer buffering;

				buffering.Init(rNum1);
				buffering.SetCumlatedLen(rNum1);
				if (rNum1 < rNum2)
				{
					Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN == buffering.CheckAppendDataBufferLength(rNum2));
				}
				else
				{
					Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_BUFFER_FULL == buffering.CheckAppendDataBufferLength(rNum2));
				}
			}
		}

		TEST_METHOD(ValidateBuffer_Test)
		{
			CumBuffer::CumBuffer buffering;
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.Init());

			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_NO_DATA == buffering.ValidateBuffer((size_t)5));
			Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.Append(3, "abc"));
			for (int i = 0; i < 10000; i++)
			{
				if (i <= 3)
				{
					Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.ValidateBuffer((size_t)i));
				}
				else
				{
					Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN == buffering.ValidateBuffer((size_t)i));
				}
			}
			
		}

		TEST_METHOD(IsDataNotRotated_Test)
		{
			auto rNum1 = 0, rNum2 = 0, rNum3 = 0;

			for (int i = 0; i < 1000; ++i)
			{
				GetFourRandomNum(rNum1, rNum2, rNum3);

				auto head = rNum1;
				auto tail = rNum2;
				auto getLeng = rNum3;

				MockCumbuffer buffering;
				buffering.Init(30 * 1024);
				buffering.SeCurHead(head);
				buffering.SetCurTail(tail);

				if (head < tail)
				{
					if (head + getLeng <= tail)
					{
						Assert::IsTrue(buffering.IsDataNotRotated(getLeng) == true);
					}
					else
					{
						Assert::IsTrue(buffering.IsDataNotRotated(getLeng) == false);
						Assert::AreEqual("invalid length", buffering.GetErrMsg().c_str());
					}	
				}
				else
				{
					Assert::IsTrue(buffering.IsDataNotRotated(getLeng) == false);
				}
			}
		}

		TEST_METHOD(IsDataRotatedAndReadOneBlock_Test)
		{
			auto rNum1 = 0, rNum2 = 0, rNum3 = 0;

			for (int i = 0; i < 1000; ++i)
			{
				GetFourRandomNum(rNum1, rNum2, rNum3);

				auto head = rNum1;
				auto tail = rNum2;
				auto getLeng = rNum3;
				auto capacity = 30 * 1024;
				MockCumbuffer buffering;
				buffering.Init(capacity);
				buffering.SeCurHead(head);
				buffering.SetCurTail(tail);

				if (head < tail)
				{
					Assert::IsTrue(buffering.IsDataRotatedAndReadOneBlock(getLeng) == false);
					
				}
				else
				{
					if (head + getLeng <= capacity)
					{
						Assert::IsTrue(buffering.IsDataRotatedAndReadOneBlock(getLeng) == true);
					}
					else
					{
						Assert::IsTrue(buffering.IsDataRotatedAndReadOneBlock(getLeng) == false);
					}
				}
			}
		}

		TEST_METHOD(IsDataRotatedAndReadTwoBlock)
		{
			auto rNum1 = 0, rNum2 = 0, rNum3 = 0;

			for (int i = 0; i < 1000; ++i)
			{
				GetFourRandomNum(rNum1, rNum2, rNum3);

				auto head = rNum1;
				auto tail = rNum2;
				auto getLeng = rNum3;
				auto capacity = 30 * 1024;
				MockCumbuffer buffering;
				buffering.Init(capacity);
				buffering.SeCurHead(head);
				buffering.SetCurTail(tail);

				if (head < tail)
				{
					Assert::IsTrue(buffering.IsDataRotatedAndReadTwoBlock(getLeng) == false);
				}
				else
				{
					if (capacity < head + getLeng)
					{
						auto firstBlock = capacity - head;
						auto seconBlock = getLeng - firstBlock;
						if (tail >= seconBlock)
						{
							Assert::IsTrue(buffering.IsDataRotatedAndReadTwoBlock(getLeng) == true);
						}
						else
						{
							Assert::IsTrue(buffering.IsDataRotatedAndReadTwoBlock(getLeng) == false);
							Assert::AreEqual("invalid length", buffering.GetErrMsg().c_str());
						}

					}
					else
					{
						Assert::IsTrue(buffering.IsDataRotatedAndReadTwoBlock(getLeng) == false);
					}
				}
			}
		}

		TEST_METHOD(ConsumeData_TestHeadTail)
		{
			auto rNum1 = 0, rNum2 = 0, rNum3 = 0;

			for (int i = 0; i < 100; ++i)
			{
				GetFourRandomNum(rNum1, rNum2, rNum3);

				auto head = rNum1;
				auto tail = rNum2;
				auto getLeng = rNum3;
				auto capacity = 30 * 1024;
				MockCumbuffer buffering;
				buffering.Init(capacity);
				buffering.SeCurHead(head);
				buffering.SetCurTail(tail);

				if (head<tail)
				{
					auto savedLen = tail - head;
					buffering.SetCumlatedLen(savedLen);
					if (getLeng <= savedLen)
					{
						Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.ConsumeData(getLeng));
						Assert::IsTrue(buffering.GetCumulatedLen() == (size_t)(savedLen- getLeng));
						Assert::IsTrue(buffering.GetCurHeadPos() == (uint64_t)(head + getLeng));
					}
					else
					{
						auto result = buffering.ConsumeData(getLeng);
						Assert::AreEqual((int)CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN, (int)result);
					}
				}
				else if(head == tail)
				{
					if ((rNum1 - rNum2) % 2 == 0)
					{
						auto savedLen = 0;
						buffering.SetCumlatedLen(savedLen);
						Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_NO_DATA == buffering.ConsumeData(getLeng));
					}
					else
					{
						auto savedLen = capacity;
						buffering.SetCumlatedLen(savedLen);
						if (getLeng <= savedLen)
						{
							Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.ConsumeData(getLeng));
							Assert::IsTrue(buffering.GetCumulatedLen() == (size_t)(savedLen - getLeng));
							Assert::IsTrue(buffering.GetCurHeadPos() == (uint64_t)(head + getLeng));
						}
						else
						{
							auto result = buffering.ConsumeData(getLeng);
							Assert::AreEqual((int)CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN, (int)result);
				
						}
					}
					
				}
				else
				if(head>tail)
				{
					auto savedLen = tail + capacity - head;
					buffering.SetCumlatedLen(savedLen);
					auto firstBlock = capacity - head;
					auto seconBlock = getLeng - firstBlock;
					if (getLeng <= savedLen)
					{
						if (seconBlock <= 0)
						{
							Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.ConsumeData(getLeng));
							Assert::IsTrue(buffering.GetCumulatedLen() == (size_t)(savedLen - getLeng));
							Assert::IsTrue(buffering.GetCurHeadPos() == (uint64_t)(head + getLeng));
						}
						else
						{
							Assert::IsTrue(CumBuffer::OP_RESULT::OP_RSLT_OK == buffering.ConsumeData(getLeng));
							Assert::IsTrue(buffering.GetCumulatedLen() == (size_t)(savedLen - getLeng));
							Assert::IsTrue(buffering.GetCurHeadPos() == (uint64_t)(seconBlock));
						}
						
					}
					else
					{
						auto result = buffering.ConsumeData(getLeng);
						Assert::AreEqual((int)CumBuffer::OP_RESULT::OP_RSLT_INVALID_LEN, (int)result);
					}
				}
				
			}
		}

	};




}