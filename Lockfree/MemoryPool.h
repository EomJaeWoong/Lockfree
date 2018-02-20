/*---------------------------------------------------------------

procademy MemoryPool.

메모리 풀 클래스.
특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

- 사용법.

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData 사용

MemPool.Free(pData);


!.	아주 자주 사용되어 속도에 영향을 줄 메모리라면 생성자에서
Lock 플래그를 주어 페이징 파일로 복사를 막을 수 있다.
아주 중요한 경우가 아닌이상 사용 금지.



주의사항 :	단순히 메모리 사이즈로 계산하여 메모리를 할당후 메모리 블록을 리턴하여 준다.
클래스를 사용하는 경우 클래스의 생성자 호출 및 클래스정보 할당을 받지 못한다.
클래스의 가상함수, 상속관계가 전혀 이뤄지지 않는다.
VirtualAlloc 으로 메모리 할당 후 memset 으로 초기화를 하므로 클래스정보는 전혀 없다.


----------------------------------------------------------------*/
#ifndef __MEMORYPOOL_H__
#define __MEMORYPOOL_H__

#include <assert.h>
#include <new.h>

template <class DATA>
class CMemoryPool
{
private:

	/* **************************************************************** */
	// 각 블럭 앞에 사용될 노드 구조체.
	/* **************************************************************** */
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			stpNextBlock = NULL;
		}

		st_BLOCK_NODE *stpNextBlock;
	};

	/* **************************************************************** */
	// 락프리 메모리풀 탑 노드.
	/* **************************************************************** */
	struct st_TOP_NODE
	{
		st_BLOCK_NODE	*pTopNode;
		__int64			iUniqueNum;
	};


public:

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 최대 블럭 개수.
	//				(bool) 메모리 Lock 플래그 - 중요하게 속도를 필요로 한다면 Lock.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(bool bLockFlag = false)
	{
		//////////////////////////////////////////////////////////////////////
		// 초기화
		//////////////////////////////////////////////////////////////////////
		_lBlockCount = 0;
		_lAllocCount = 0;
		_bLockflag = bLockFlag;

		//////////////////////////////////////////////////////////////////////
		// 탑 노드 할당
		//////////////////////////////////////////////////////////////////////
		_pTop = (st_TOP_NODE *)_aligned_malloc(sizeof(st_TOP_NODE), 16);
		_pTop->pTopNode = nullptr;
		_pTop->iUniqueNum = 0;

		_iUniqueNum = 0;
	}

	virtual					~CMemoryPool()
	{
		st_BLOCK_NODE *pNode;

		for (int iCnt = 0; iCnt < _lBlockCount; iCnt++)
		{
			pNode = _pTop->pTopNode;
			_pTop->pTopNode = _pTop->pTopNode->stpNextBlock;
			free(pNode);
		}	
	}


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA					*Alloc(bool bPlacementNew = true)
	{
		st_BLOCK_NODE	*pAllocNode = nullptr;
		st_TOP_NODE		stCloneTopNode;

		DATA			*pData;

		long			lBlockCount = _lBlockCount;
		InterlockedIncrement((long *)&_lAllocCount);
	
		//////////////////////////////////////////////////////////////////////
		// 할당 해야 할 경우
		//////////////////////////////////////////////////////////////////////
		if (lBlockCount < _lAllocCount)
		{
			pAllocNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
			InterlockedIncrement((long *)&_lBlockCount);
		}

		else
		{
			__int64 iUniqueNum = InterlockedIncrement((long *)&_iUniqueNum);

			do
			{
				stCloneTopNode.iUniqueNum	= _pTop->iUniqueNum;
				stCloneTopNode.pTopNode		= _pTop->pTopNode;
			} while (!InterlockedCompareExchange128(
				(LONG64 *)_pTop,
				iUniqueNum,
				(LONG64)_pTop->pTopNode->stpNextBlock,
				(LONG64 *)&stCloneTopNode
				));

			pAllocNode = stCloneTopNode.pTopNode;
		}

		pData = (DATA *)(pAllocNode + 1);

		if (bPlacementNew)
			new (pData)DATA;

		return pData;
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool					Free(DATA *pData)
	{
		st_BLOCK_NODE	*pReturnNode = ((st_BLOCK_NODE *)pData) - 1;
		st_TOP_NODE		stCloneTopNode;

		__int64			iUniqueNum = InterlockedIncrement((long *)&_iUniqueNum);

		do
		{
			stCloneTopNode.iUniqueNum	= _pTop->iUniqueNum;
			stCloneTopNode.pTopNode		= _pTop->pTopNode;

			pReturnNode->stpNextBlock	= _pTop->pTopNode;
		} while (!InterlockedCompareExchange128(
			(LONG64 *)_pTop,
			iUniqueNum,
			(LONG64)pReturnNode,
			(LONG64 *)&stCloneTopNode
			));

		InterlockedDecrement((long *)&_lAllocCount);

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int						GetAllocCount(void) { return (int)_lAllocCount; }


	//////////////////////////////////////////////////////////////////////////
	// 메모리풀 락
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void					Lock()
	{
		AcquireSRWLockExclusive(&_srwMemoryPool);
	}


	//////////////////////////////////////////////////////////////////////////
	// 메모리풀 언락
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void					Unlock()
	{
		ReleaseSRWLockExclusive(&_srwMemoryPool);
	}

private:
	//////////////////////////////////////////////////////////////////////////
	// MemoryPool의 Top
	//////////////////////////////////////////////////////////////////////////
	st_TOP_NODE				*_pTop;


	//////////////////////////////////////////////////////////////////////////
	// Top의 Unique Number
	//////////////////////////////////////////////////////////////////////////
	__int64					_iUniqueNum;


	//////////////////////////////////////////////////////////////////////////
	// Lockflag
	//////////////////////////////////////////////////////////////////////////
	bool					_bLockflag;


	//////////////////////////////////////////////////////////////////////////
	// 현재 할당중인 블록 수
	//////////////////////////////////////////////////////////////////////////
	long					_lAllocCount;


	//////////////////////////////////////////////////////////////////////////
	// 전체 블록 수
	//////////////////////////////////////////////////////////////////////////
	long					_lBlockCount;


	//////////////////////////////////////////////////////////////////////////
	// 동기화 객체
	//////////////////////////////////////////////////////////////////////////
	SRWLOCK					_srwMemoryPool;
};


#endif