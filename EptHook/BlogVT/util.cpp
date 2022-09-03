#include"def.h"

BOOLEAN CheckVTSupport()
{
	int ctx[4] = { 0 };

	//��ȡCPU��Ϣ������ɹ�,ctx�зֱ����eax��edx����Ϣ
	__cpuidex(ctx, 1, 0);

	//���ecx�ĵ���λ�Ƿ�Ϊ0,0��ʾ��CPU��֧��VT,IA32�ֲ��3C 23.6
	if ((ctx[2] & (1 << 5)) == 0)
	{
		//��֧�����⻯
		return FALSE;
	}

	return TRUE;
}

BOOLEAN CheckVTEnable()
{
	ULONG_PTR msr;
	msr = __readmsr(0x3A);

	//����0λ�Ƿ�Ϊ0,Ҳ����BIOS��VT�Ƿ���,0�ǹر�(��IA32�ֲ��3C 23.7,��ΪWindowsϵͳ��������ڱ���ģʽ��,���Բ���Ҫ���CR0��)
	if ((msr & 1) == 0)
		return FALSE;

	return TRUE;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void* __cdecl operator new(size_t size) {
	if (size == 0) {
		size = 1;
	}
	PVOID ptr = ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'vt');
	if (ptr == NULL) {
		Log("new���������ڴ�ʧ�ܣ�");
	}
	return ptr;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void __cdecl operator delete(void* p, SIZE_T size) {
	UNREFERENCED_PARAMETER(size);
	if (p) {
		ExFreePoolWithTag(p, 'vt');
	}
}

PVOID kmalloc(ULONG_PTR size)
{
	PHYSICAL_ADDRESS MaxAddr = { 0 };
	MaxAddr.QuadPart = -1;
	PVOID addr = MmAllocateContiguousMemory(size, MaxAddr);
	if (addr)
		RtlSecureZeroMemory(addr, size);
	else
		Log("�����ڴ�ʧ��");
	return addr;
}

void kfree(PVOID p)
{
	if (p) MmFreeContiguousMemory((PVOID)p);
}