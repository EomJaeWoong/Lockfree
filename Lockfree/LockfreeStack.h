#ifndef __LOCKFREESTACK__H__
#define __LOCKFREESTACK__H__

template <class DATA>
class CLockfreeStack
{
private :
	/*----------------------------------------------------------------------*/
	// Lockfree Stack�� ��� ����ü
	/*----------------------------------------------------------------------*/
	struct st_NODE
	{
		DATA		Data;
		st_NODE		*pNext;
	};

	/*----------------------------------------------------------------------*/
	// Lockfree Stack�� ž ��� ����ü
	/*----------------------------------------------------------------------*/
	struct st_TOP_NODE
	{
		st_NODE		*pTopNode;
		__int64		iUniqueNum;
	};



public :
	//////////////////////////////////////////////////////////////////////////
	// ������
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
	// �Ҹ���
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
	// Stack ������ ����
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
	// stack���� ������ ����
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
	// ������
	//////////////////////////////////////////////////////////////////////////
	bool		isEmpty(void)
	{

	}


	//////////////////////////////////////////////////////////////////////////
	// ������
	//////////////////////////////////////////////////////////////////////////
	long		GetUseSize(void) {	return _lUseSize;	 }


private :
	//////////////////////////////////////////////////////////////////////////
	// ������
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool<st_NODE>		*_pStackPool;


	//////////////////////////////////////////////////////////////////////////
	// ������
	//////////////////////////////////////////////////////////////////////////
	st_TOP_NODE					*_pTop;


	//////////////////////////////////////////////////////////////////////////
	// ������
	//////////////////////////////////////////////////////////////////////////
	__int64						_iUniqueNum;


	//////////////////////////////////////////////////////////////////////////
	// ������
	//////////////////////////////////////////////////////////////////////////
	long						_lUseSize;
};

#endif