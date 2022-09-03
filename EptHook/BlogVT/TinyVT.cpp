#include"TinyVT.h"

TinyVT::TinyVT(int index)
{
	this->index = index;
	VMX_Region = (ULONG_PTR)kmalloc(PAGE_SIZE);
	VMCS_Region = (ULONG_PTR)kmalloc(PAGE_SIZE);
	MsrBitmap = (ULONG_PTR)kmalloc(PAGE_SIZE);
	VmmStack = (PCHAR)kmalloc(VMM_STACK_SIZE);
	Log("��ǰCPU:%d",index);
}

TinyVT::~TinyVT()
{
	Cr4 cr4 = { 0 };
	cr4.all = __readcr4();
	if (cr4.fields.vmxe)
	{
		cr4.fields.vmxe = FALSE;
		__writecr4(cr4.all);
	}

	kfree((PVOID)VMX_Region);
	kfree((PVOID)VMCS_Region);
	kfree((PVOID)MsrBitmap);
	kfree(VmmStack);

	//��ִ��vmxoffָ��֮�󣬲���ȥ������ΪFALSE��
	//Ia32FeatureControlMsr msr = { 0 };
	//msr.all = __readmsr(MsrFeatureControl);
	//if (msr.fields.lock)
	//{
	//	msr.fields.lock = FALSE;
	//	msr.fields.enable_vmxon = FALSE;
	//	__writemsr(MsrFeatureControl, msr.all);
	//	msr.all = __readmsr(MsrFeatureControl);
	//}
}

BOOLEAN TinyVT::StartVT()
{
	isEnable = ExecuteVMXON();
	if (!isEnable) {
		Log("[CPU;%d]VMXONʧ�ܣ�", index);
		return FALSE;
	}

	FunAddr funAddr = { 0 };
	funAddr.fun = &TinyVT::InitVMCS;

	isEnable = AsmVmxLaunch(funAddr.addr, this);

	return isEnable;
}

BOOLEAN TinyVT::ExecuteVMXON()
{
	//���汾��
#pragma warning(push)
#pragma warning(disable:4244)
	*(ULONG*)VMX_Region = __readmsr(MsrVmxBasic);
	*(ULONG*)VMCS_Region = __readmsr(MsrVmxBasic);
#pragma warning(pop)

	//����CR4
	Cr4 cr4 = { 0 };
	cr4.all = __readcr4();
	cr4.fields.vmxe = TRUE;
	__writecr4(cr4.all);

	//��ÿ��cpu����vmxonָ�������
	Ia32FeatureControlMsr msr = { 0 };
	msr.all = __readmsr(MsrFeatureControl);
	if (!msr.fields.lock)
	{
		msr.fields.lock = TRUE;
		msr.fields.enable_vmxon = TRUE;
		__writemsr(MsrFeatureControl, msr.all);
		msr.all = __readmsr(MsrFeatureControl);
	}

	//ִ��VMXON
	ULONG_PTR phyaddr = MmGetPhysicalAddress((PVOID)VMX_Region).QuadPart;
	__vmx_on(&phyaddr);

	FlagRegister eflags = { 0 };

	*(ULONG_PTR*)(&eflags) = __readeflags();
	if (eflags.fields.cf != 0) {
		Log("[CPU:%d]VMXONִ��ʧ�ܣ�",index);
		return FALSE;
	}

	phyaddr = MmGetPhysicalAddress((PVOID)VMCS_Region).QuadPart;
	//��ʼ��VMCS����
	__vmx_vmclear(&phyaddr);
	//ѡ�е�ǰVMCS����Ϊ���VMCS������׼��
	__vmx_vmptrld(&phyaddr);
	return TRUE;
}
