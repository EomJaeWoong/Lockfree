// main.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

/*-------------------------------------------------------------------------------------------*/
//
// 락프리 모델 테스트 프로그램 제작
//
// - MemoryPool, Lockfree Stack, Lockfree Queue
//
// 여러개의 스레드에서 일정수량의 Alloc 과 Free 를 반복적으로 함
// 모든 데이터는 0x0000000055555555 으로 초기화 되어 있음.
//
// 사전에 100,000 개의 st_TEST_DATA  데이터를 메모리풀에 넣어둠. 
//  모든 데이터는 Data - 0x0000000055555555 / Cound - 0 초기화.
//
/*-------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------*/
// 락프리 메모리풀
/*-------------------------------------------------------------------------------------------*/
CMemoryPool<st_TEST_DATA>		g_MemoryPool;


/*-------------------------------------------------------------------------------------------*/
// 락프리 스택
/*-------------------------------------------------------------------------------------------*/
CLockfreeStack<st_TEST_DATA *>	g_LockfreeStack;


/*-------------------------------------------------------------------------------------------*/
// 락프리 스택
/*-------------------------------------------------------------------------------------------*/
CLockfreeQueue<st_TEST_DATA *>	g_LockfreeQueue;


/*-------------------------------------------------------------------------------------------*/
// 프로파일링 변수들
/*-------------------------------------------------------------------------------------------*/
long							lInTPS = 0;
long							lOutTPS = 0;

long							lInCounter = 0;
long							lOutCounter = 0;



/*-------------------------------------------------------------------------------------------*/
// 쓰레드 핸들
/*-------------------------------------------------------------------------------------------*/
HANDLE							hThread[dfTHREAD_MAX];
DWORD							dwThreadID;



BOOL							g_bShutdown;

/*===========================================================================================*/






/*-------------------------------------------------------------------------------------------*/
// 락프리 메모리풀 테스트 
/*-------------------------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////////////////////
// 쓰레드
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned __stdcall				MemoryPoolThread(void *pParam)
{
	st_TEST_DATA *pDataArray[dfTHREAD_ALLOC];

	while (!g_bShutdown){
		Sleep(10);

		///////////////////////////////////////////////////////////////////////////////////////
		// Alloc (10000개)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			pDataArray[iCnt] = g_MemoryPool.Alloc();
			InterlockedIncrement((long *)&lOutCounter);
		}

		Sleep(20);

		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked + 1 (Data + 1 / Count + 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		//Sleep(dfSLEEP);


		///////////////////////////////////////////////////////////////////////////////////////
		// 여전히 0x000000005555556, 1이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555556 != pDataArray[iCnt]->lData) ||
				(1 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked - 1 (Data - 1 / Count - 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		//Sleep(dfSLEEP);


		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 / 0 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}

		Sleep(20);

		///////////////////////////////////////////////////////////////////////////////////////
		// Free
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_MemoryPool.Free(pDataArray[iCnt]);
			InterlockedIncrement((long *)&lInCounter);
		}	
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 초기화
///////////////////////////////////////////////////////////////////////////////////////////////
void							InitMemoryPool()
{
	st_TEST_DATA *pDataArray[dfDATA_MAX];

	///////////////////////////////////////////////////////////////////////////////////////////
	// Alloc (10000개)
	// 데이터 생성(확보)
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		pDataArray[iCnt] = g_MemoryPool.Alloc();


	///////////////////////////////////////////////////////////////////////////////////////////
	// iData = 0x0000000055555555 셋팅
	// lCount = 0 셋팅
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
	{
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// 2. 메모리풀에 넣음(반환)
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		g_MemoryPool.Free(pDataArray[iCnt]);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 테스트 화면
///////////////////////////////////////////////////////////////////////////////////////////////
void							TestMemoryPool()
{
	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			MemoryPoolThread,
			(LPVOID)0,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	wprintf(L"=====================================================================\n");
	wprintf(L"                        MemoryPool Testing...                        \n");
	wprintf(L"=====================================================================\n\n");

	while (1)
	{
		wprintf(L"Use Count		: %d, ", g_MemoryPool.GetAllocCount());
		wprintf(L"Block Count		: %d\n", g_MemoryPool.GetBlockCount());

		if (g_MemoryPool.GetAllocCount() > (dfTHREAD_MAX * dfTHREAD_ALLOC))
			CCrashDump::Crash();

		Sleep(999);
	}

}
/*-------------------------------------------------------------------------------------------*/





/*-------------------------------------------------------------------------------------------*/
// 락프리 스택 테스트 
/*-------------------------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////////////////////
// 쓰레드
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned __stdcall			LockfreeStackThread(LPVOID pParam)
{
	st_TEST_DATA *pDataArray[dfTHREAD_ALLOC];

	while (1){
		Sleep(10);

		///////////////////////////////////////////////////////////////////////////////////////
		// 뽑기
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if(!g_LockfreeStack.Pop(&pDataArray[iCnt]))
				CCrashDump::Crash();

			InterlockedIncrement(&lOutCounter);
		}

		

		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}

		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked + 1 (Data + 1 / Count + 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		Sleep(0);


		///////////////////////////////////////////////////////////////////////////////////////
		// 여전히 0x000000005555556, 1이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555556 != pDataArray[iCnt]->lData) ||
				(1 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked - 1 (Data - 1 / Count - 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		//Sleep(0);

		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}

		Sleep(20);

		///////////////////////////////////////////////////////////////////////////////////////
		// Free
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_LockfreeStack.Push(pDataArray[iCnt]);
			InterlockedIncrement(&lInCounter);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 초기화
///////////////////////////////////////////////////////////////////////////////////////////////
void			InitLockfreeStack()
{
	st_TEST_DATA *pDataArray[dfDATA_MAX];

	///////////////////////////////////////////////////////////////////////////////////////////
	// 데이터 생성(확보)
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		pDataArray[iCnt] = new st_TEST_DATA;


	///////////////////////////////////////////////////////////////////////////////////////////
	// iData = 0x0000000055555555 셋팅
	// lCount = 0 셋팅
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
	{
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// 2.스택에 넣음
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		g_LockfreeStack.Push(pDataArray[iCnt]);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 테스트 출력 
///////////////////////////////////////////////////////////////////////////////////////////////
void			TestLockfreeStack()
{
	long		lUseTPS = 0;
	long		lAllocTPS = 0;

	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			LockfreeStackThread,
			(LPVOID)0,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	wprintf(L"=====================================================================\n");
	wprintf(L"                       Lockfree Stack Testing...                     \n");
	wprintf(L"=====================================================================\n\n");

	while (1)
	{
		lInTPS = lInCounter;
		lOutTPS = lOutCounter;
		lUseTPS = g_LockfreeStack.GetUseSize();
		lAllocTPS = lAllocTPS > g_LockfreeStack.GetAllocSize() ? lAllocTPS : g_LockfreeStack.GetAllocSize();

		lInCounter = 0;
		lOutCounter = 0;

		wprintf(L"Push : %8ld, ", lInTPS);
		wprintf(L"Pop : %8ld, ", lOutTPS);
		wprintf(L"Size : %8ld, ", lUseTPS);
		wprintf(L"Alloc	: %8ld\n", lAllocTPS);

		if ((g_LockfreeStack.GetUseSize() > (dfTHREAD_MAX * dfTHREAD_ALLOC)) ||
			(g_LockfreeStack.GetAllocSize() > (dfTHREAD_MAX * dfTHREAD_ALLOC)))
			CCrashDump::Crash();

		Sleep(999);
	}


}
/*-------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------*/
// 락프리 큐 테스트
/*-------------------------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////////////////////
// 쓰레드
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned __stdcall			LockfreeQueueThread(LPVOID pParam)
{
	st_TEST_DATA *pDataArray[dfTHREAD_ALLOC];

	while (1){
		Sleep(10);

		///////////////////////////////////////////////////////////////////////////////////////
		// 뽑기
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if(!g_LockfreeQueue.Get(&pDataArray[iCnt]))
				CCrashDump::Crash();
			InterlockedIncrement((long *)&lOutCounter);
		}

		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}

		

		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked + 1 (Data + 1 / Count + 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		//Sleep(dfSLEEP);


		///////////////////////////////////////////////////////////////////////////////////////
		// 여전히 0x000000005555556, 1이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555556 != pDataArray[iCnt]->lData) ||
				(1 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked - 1 (Data - 1 / Count - 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		//Sleep(dfSLEEP);

		

		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 / 0 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}

		Sleep(20);

		///////////////////////////////////////////////////////////////////////////////////////
		// Free
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_LockfreeQueue.Put(pDataArray[iCnt]);
			InterlockedIncrement((long *)&lInCounter);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 초기화
///////////////////////////////////////////////////////////////////////////////////////////////
void			InitLockfreeQueue()
{
	st_TEST_DATA *pDataArray[dfDATA_MAX];

	///////////////////////////////////////////////////////////////////////////////////////////
	// 데이터 생성(확보)
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		pDataArray[iCnt] = new st_TEST_DATA;


	///////////////////////////////////////////////////////////////////////////////////////////
	// iData = 0x0000000055555555 셋팅
	// lCount = 0 셋팅
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
	{
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// 큐에 넣음
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		g_LockfreeQueue.Put(pDataArray[iCnt]);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 테스트 출력 
///////////////////////////////////////////////////////////////////////////////////////////////
void			TestLockfreeQueue()
{
	long		lUseTPS = 0;
	long		lAllocTPS = 0;

	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			LockfreeQueueThread,
			(LPVOID)0,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	system("cls");
	wprintf(L"=====================================================================\n");
	wprintf(L"                       Lockfree Queue Testing...                     \n");
	wprintf(L"=====================================================================\n\n");

	while (1)
	{
		lInTPS = lInCounter;
		lOutTPS = lOutCounter;
		lUseTPS = g_LockfreeQueue.GetUseSize();
		lAllocTPS = lAllocTPS > g_LockfreeQueue.GetAllocSize() ? lAllocTPS : g_LockfreeQueue.GetAllocSize();

		lInCounter = 0;
		lOutCounter = 0;

		wprintf(L"Put : %8ld, ", lInTPS);
		wprintf(L"Get : %8ld, ", lOutTPS);
		wprintf(L"Size : %8ld, ", lUseTPS);
		wprintf(L"Alloc : %8ld\n", lAllocTPS);

		if ((g_LockfreeQueue.GetUseSize() > (dfTHREAD_MAX * dfTHREAD_ALLOC)) ||
			(g_LockfreeQueue.GetAllocSize() > (dfTHREAD_MAX * dfTHREAD_ALLOC + 1)))
			CCrashDump::Crash();

		Sleep(999);
	}

}
/*-------------------------------------------------------------------------------------------*/




int				_tmain(int argc, _TCHAR* argv[])
{
	CCrashDump::CCrashDump();

	char		chSelectModel;
	
	g_bShutdown = FALSE;

	do
	{
		system("cls");

		wprintf(L"=====================================================================\n");
		wprintf(L"                         Lockfree Model Test                         \n");
		wprintf(L"=====================================================================\n\n");

		wprintf(L"1. Lockfree MemoryPool\n\n");
		wprintf(L"2. Lockfree Stack\n\n");
		wprintf(L"3. Lockfree Queue\n\n");

		wprintf(L"=====================================================================\n");

		scanf_s("%c", &chSelectModel, sizeof(chSelectModel));
		fflush(stdin);

		switch (chSelectModel)
		{
		case '1':
			InitMemoryPool();
			TestMemoryPool();
			break;

		case '2':
			InitLockfreeStack();
			TestLockfreeStack();
			break;

		case '3':
			InitLockfreeQueue();
			TestLockfreeQueue();
			break;

		case 'q' :
		case 'Q' :
			g_bShutdown = TRUE;
			break;

		default:
			continue;
			break;
		}

	} while (0);

	return 0;
}