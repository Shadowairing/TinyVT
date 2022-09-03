#include"TinyVT.h"

//һ��Ҫ��������ڴ�
#define TOTAL_MEM 32

//ָ������EPT��ָ��
Eptp EptP = { 0 };
//��������ڴ���׵�ַ
PCHAR EptMem = 0;
//��ʱ��EPT�����Ǳ���ģ��������Ƿ�����EPT
BOOLEAN UseEpt = TRUE;

//ͨ�������ַ��ȡ��Ӧ��PTE
EptCommonEntry* GetPteByPhyAddr(ULONG_PTR addr)
{
	//����9 9 9 9 12 ��ҳ��ȡ��������±�
	ULONG_PTR PDPT_Index = (addr >> (9 + 9 + 12)) & 0x1FF;
	ULONG_PTR PDT_Index = (addr >> (9 + 12)) & 0x1FF;
	ULONG_PTR PT_Index = (addr >> 12) & 0x1FF;

	//���ÿһ�ȷ֣�ÿһ�ȷ���һ��PDT+512��PT���ų�����ҳ
	ULONG_PTR offset = (TOTAL_MEM + TOTAL_MEM * 512) / 513;
	//�õ�Ŀ��ȷ֣���һҳ��PDT����Ҫ
	offset = offset * PDPT_Index + 1;
	//�õ���ӦPT
	offset = offset + PDT_Index;

	ULONG_PTR* PTE = (ULONG_PTR*)(EptMem + offset * PAGE_SIZE) + PT_Index;
	return (EptCommonEntry*)PTE;
}


BOOLEAN InitEpt()
{
	if (!UseEpt) return FALSE;
	ULONG_PTR index = 0;
	ULONG_PTR* PML4T = 0, * PDPT = 0;

	//�����ܵ��ڴ棬����2����PML4T��PDPT��Ҫ����ҳ�ڴ棬TOTAL_MEM��PDT��TOTAL_MEM * 512��PT
	EptMem = (PCHAR)kmalloc((2 + TOTAL_MEM + TOTAL_MEM * 512) * PAGE_SIZE);
	if (!EptMem) return FALSE;

	//�����ҳ��PML4T��PDPT����������һ��ÿ���СΪ4KB�����飬��һ��Ϊ(EptMem + 0 * PAGE_SIZE)
	//���һ��Ϊ(EptMem + (1 + TOTAL_MEM + TOTAL_MEM * 512) * PAGE_SIZE)
	PML4T = (ULONG_PTR*)(EptMem + (TOTAL_MEM + TOTAL_MEM * 512) * PAGE_SIZE);
	PDPT = (ULONG_PTR*)(EptMem + (1 + TOTAL_MEM + TOTAL_MEM * 512) * PAGE_SIZE);

	/*
	�ܵ��ڴ沼�־��Ǿ���
	EptMem = {[PDT+512��PT],[PDT+512��PT],[PDT+512��PT]...��32��[PDT+512��PT]��PML4T��PDPT}
	*/

	//��7��ʾ�ɶ���д��ִ��Ȩ��
	PML4T[0] = MmGetPhysicalAddress(PDPT).QuadPart + 7;
	for (ULONG_PTR PDPT_Index = 0; PDPT_Index < TOTAL_MEM; PDPT_Index++)
	{
		//����һҳ��PDT
		ULONG_PTR* PDT = (ULONG_PTR*)(EptMem + PAGE_SIZE * index++);
		//��ʼ��PDPTE
		PDPT[PDPT_Index] = MmGetPhysicalAddress(PDT).QuadPart + 7;

		for (ULONG_PTR PDT_Index = 0; PDT_Index < 512; PDT_Index++)
		{
			//����һҳ��PT
			ULONG_PTR* PT = (ULONG_PTR*)(EptMem + PAGE_SIZE * index++);
			//��ʼ��PDE
			PDT[PDT_Index] = MmGetPhysicalAddress(PT).QuadPart + 7;

			for (ULONG_PTR PT_Index = 0; PT_Index < 512; PT_Index++)
			{
				//��ʼ��PTE
				PT[PT_Index] = (PDPT_Index * (1 << 30) + PDT_Index * (1 << 21) + PT_Index * (1 << 12) + 0x37);
			}
		}
	}

	//IA32�ֲ�24.6.11
	EptP.all = MmGetPhysicalAddress(PML4T).QuadPart + 7;

	//��0��6
	ULONGLONG memory_type = __readmsr(MsrVmxEptVpidCap);
	//�������Ƶ�100000000�������㣬������8λ�Ƿ�Ϊ1,Ϊ1������0��������6
	memory_type &= 0x100;
	EptP.fields.memory_type = memory_type ? 0:6;

	//ʹ�õ�ҳ��ļ���-1��������9 9 9 9 12 ��ҳ��4��
	EptP.fields.page_walk_length = 3;

	return TRUE;
}