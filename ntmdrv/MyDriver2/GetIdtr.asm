.code

SwitchToRing0 PROC
	pushad
	pushfd
	push fs
	mov bx, 0x30;
	mov fs, bx
	push ds
	push es
	ret
SwitchToRing0 ENDP

ReverseRingX PROC
	pop es;
	pop ds;
	pop fs;
	popfd;
	popad;
	jmp rcx; //RCX = %1
	ret
ReverseRingX ENDP

DisableWriteProtect PROC
      push eax
      mov eax, cr0
      mov DWORD PTR[rcx], eax
      and eax, 0FFFEFFFFh; // CR0 16 BIT = 0
      mov cr0, eax
      pop eax
	  ret
DisableWriteProtect ENDP
  
EnableWriteProtect PROC
      push eax
      mov eax, ecx; //恢复原有 CR0 属性
      mov cr0, eax
      pop eax
	  ret
EnableWriteProtect ENDP

END

;
;VS 64bit C/C++ invoke ASM function,the arguments saved in RCX/RDX/R8/R9, and Push on stack.
;