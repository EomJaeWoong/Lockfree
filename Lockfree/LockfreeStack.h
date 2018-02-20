#ifndef __LOCKFREESTACK__H__
#define __LOCKFREESTACK__H__

template <class DATA>
class CLockfreeStack
{
private :
	/*----------------------------------------------------------------------*/
	// Lockfree Stack의 노드 구조체
	/*----------------------------------------------------------------------*/
	struct st_NODE
	{
		DATA		Data;
		st_NODE		*pNext;
	};

	/*----------------------------------------------------------------------*/
	// Lockfree Stack의 탑 노드 구조체
	/*----------------------------------------------------------------------*/
	struct st_TOP_NODE
	{
		st_NODE		*pTopNode;
		__int64		iUniqueNum;
	};



public :
	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
				CLockfreeStack()
	{
		_lUseSize		= 0;
		_iUniqueNum		= 0;

		_pStackPool		= new CMemoryPool<st_NODE>;

		_pTop			= (st_TOP_NODE *)_aligned_malloc(sizeof(st_TOP_NODE), 16);

		_pTop->pTopNode = nullptr;
		_pTop->iUniqueNum = 0;
	}


	//////////////////////////////////////////////////////////////////////////
	// 소멸자
	//////////////////////////////////////////////////////////////////////////
	virtual		~CLockfreeStack()
	{
		st_NODE *pDeleteNode = nullptr;

		for (int iCnt = 0; iCnt < _lUseSize; iCnt++)
		{
			pDeleteNode			= _pTop->pTopNode;
			_pTop->pTopNode		= _pTop->pTopNode->pNext;
			_pStackPool->Free(pDeleteNode);
		}

		delete _pStackPool;
		_aligned_free(_pTop);
	}


	//////////////////////////////////////////////////////////////////////////
	// Stack 데이터 삽입
	//////////////////////////////////////////////////////////////////////////
	bool		Push(DATA Data)
	{
		st_TOP_NODE stCloneTopNode;
		st_NODE		*pNode		= _pStackPool->Alloc();
		if (nullptr == pNode)
			return false;

		pNode->Data = Data;

		__int64		iUniqueNum  = InterlockedIncrement64((LONG64 *)&_iUniqueNum);

		do
		{
			stCloneTopNode.iUniqueNum	= _pTop->iUniqueNum;
			stCloneTopNode.pTopNode		= _pTop->pTopNode;

			pNode->pNext				= _pTop->pTopNode;
		} while (!InterlockedCompareExchange128(
			(LONG64 *)_pTop,
			iUniqueNum,
			(LONG64)pNode,
			(LONG64 *)&stCloneTopNode
			));

		InterlockedIncrement((long *)&_lUseSize);

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// stack에서 데이터 추출
	//////////////////////////////////////////////////////////////////////////
	bool		Pop(DATA *pOutData)
	{
		st_TOP_NODE stCloneTopNode;

		__int64		iUniqueNum = InterlockedIncrement64((LONG64 *)&_iUniqueNum);

		do
		{
			stCloneTopNode.iUniqueNum	= _pTop->iUniqueNum;
			stCloneTopNode.pTopNode		= _pTop->pTopNode;
		} while (!InterlockedCompareExchange128(
			(LONG64 *)_pTop,
			iUniqueNum,
			(LONG64)_pTop->pTopNode->pNext,
			(LONG64 *)&stCloneTopNode
			));

		*pOutData = stCloneTopNode.pTopNode->Data;

		_pStackPool->Free(stCloneTopNode.pTopNode);

		InterlockedDecrement((long *)&_lUseSize);

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
	bool		isEmpty(void)
	{

	}


	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
	long		GetUseSize(void) {	return _lUseSize;	 }


private :
	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool<st_NODE>		*_pStackPool;


	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
	st_TOP_NODE					*_pTop;


	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
	__int64						_iUniqueNum;


	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////
	long						_lUseSize;
};

#endif