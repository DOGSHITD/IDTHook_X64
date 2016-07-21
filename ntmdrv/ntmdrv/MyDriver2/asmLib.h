#ifndef __ASMCODE_H  
#define __ASMCODE_H  

#ifdef __cplusplus
extern "C"
{
#endif
	//unsigned long long _stdcall GetIdtr(void);
	void _stdcall SwitchToRing0(void);
	void _stdcall ReverseRingX(unsigned __int64);
	void _stdcall DisableWriteProtect(unsigned __int64 *);
	void _stdcall EnableWriteProtect(unsigned __int64);
	void _stdcall JmpAddress(unsigned __int64);
	void _stdcall SaveOriginalFunctionPtr(unsigned __int64);
	void _stdcall SaveHookFunction(unsigned __int64);
	void _stdcall HookRoutine(void);
#ifdef __cplusplus
}
#endif
#endif 