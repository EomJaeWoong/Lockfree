#ifndef __MAIN__H__
#define __LOCKFREEQUEUE_TEST__H__

struct st_TEST_DATA
{
	volatile LONG64	lData;
	volatile LONG64	lCount;
};

#define		dfTHREAD_ALLOC		3
#define		dfTHREAD_MAX		10

#define		dfDATA_MAX			dfTHREAD_ALLOC * dfTHREAD_MAX

#define		dfSLEEP				0

#endif