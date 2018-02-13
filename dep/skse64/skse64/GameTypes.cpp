#include "skse64/GameTypes.h"

BSString::~BSString()
{
	if (m_data)
	{
		Heap_Free(m_data);
		m_data = NULL;
	}
}

const char * BSString::Get(void)
{
	return m_data ? m_data : "";
}

StringCache::Ref::Ref()
{
	CALL_MEMBER_FN(this, ctor)("");
}


StringCache::Ref::Ref(const char * buf)
{
	CALL_MEMBER_FN(this, ctor)(buf);
}

void SimpleLock::Lock(void)
{
	SInt32 myThreadID = GetCurrentThreadId();
	if (threadID == myThreadID) {
		lockCount++;
		return;
	}

	UInt32 spinCount = 0;
	while (InterlockedCompareExchange(&threadID, myThreadID, 0))
		Sleep(++spinCount > kFastSpinThreshold);

    lockCount = 1;
}

void SimpleLock::Release(void)
{
	if (--lockCount == 0)
		InterlockedCompareExchange(&threadID, 0, threadID);
}

void UpdateRegistrationHolder::Order(UInt32 index)
{
	UInt32 count = m_regs.count;
	UInt32 pivotIndex = count >> 1;

	if (index >= pivotIndex)
		return;

	Registration * pOldReg = m_regs[index];
	UInt32 startIndex = index;
	UInt32 cmpIndex;
	do
	{
		cmpIndex = 2 * index + 1;

		if (cmpIndex < (count - 1))
		{
			Registration * pCur = m_regs[cmpIndex];
			Registration * pSucc = m_regs[cmpIndex + 1];
			if (!pCur || !pSucc || pCur->schedTime > pSucc->schedTime)
				cmpIndex++;
		}

		Registration * pCmpReg = m_regs[cmpIndex];
		if (!pCmpReg || !pOldReg || pCmpReg->schedTime > pOldReg->schedTime)
			break;

		m_regs[index] = pCmpReg;
		index = cmpIndex;

	} while (cmpIndex < pivotIndex);

	if (index != startIndex)
		m_regs[index] = pOldReg;

}

bool UpdateRegistrationHolder::Remove(UInt64 & handle)
{
	if (m_regs.count == 0)
		return false;

	for (UInt32 i = 0; i<m_regs.count; i++)
	{
		Registration * pReg = m_regs[i];

		if (pReg && pReg->handle == handle)
		{
			UInt32 lastIndex = m_regs.count - 1;

			// Remove last entry => no reorganizing necessary
			if (i == lastIndex)
			{
				pReg->Release();
				m_regs.count--;
			}
			else
			{
				m_regs[i] = m_regs[lastIndex];
				m_regs[lastIndex] = pReg;	// seems pointless, but the original code does it

				pReg->Release();
				m_regs.count--;

				Order(i);
			}
			return true;
		}
	}
	return false;
}
