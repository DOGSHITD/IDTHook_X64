PUBLIC	FunctionAddress
PUBLIC	OriginalFunction
PUBLIC	HookFunction
EXTRN	?oldFunction@@3_KA:QWORD

EXTRN	__imp_KeLowerIrql:PROC
EXTRN	KeGetCurrentProcessorNumber:PROC


_BSS	SEGMENT
	OriginalFunction DQ 0
	FunctionAddress DQ 0
	HookFunction DQ 0
_BSS	ENDS

.CODE

DisableWriteProtect PROC
      push rax
      mov rax, cr0
      mov [rcx], rax
      and rax, 00000000FFFEFFFFh; // CR0 16 BIT = 0
      mov cr0, rax
      pop rax
	  ret
DisableWriteProtect ENDP
  
EnableWriteProtect PROC
      push rax
      mov rax, rcx; //reverse CR0
      mov cr0, rax
      pop rax
	  ret
EnableWriteProtect ENDP

JmpAddress PROC
	jmp RCX
	ret
JmpAddress ENDP

SaveOriginalFunctionPtr PROC
	MOV FunctionAddress, RCX
	RET
SaveOriginalFunctionPtr ENDP

SaveHookFunction PROC
	MOV HookFunction, RCX
	RET
SaveHookFunction ENDP

HookRoutine1 PROC
	PUSH RAX
	PUSH RCX
	PUSH RDX
	PUSH RBX
	PUSH RSP
	PUSH RBP
	PUSH RSI
	PUSH RDI
	
	PUSHF

	PUSH R8
	PUSH R9
	PUSH R10
	PUSH R11
	PUSH R12
	PUSH R13
	PUSH R14
	PUSH R15

	PUSH FS

	MOV RBX, 30h
	MOV FS, RBX
	XOR  ECX, ECX
	CALL QWORD PTR __imp_KeLowerIrql
	CALL HookFunction
	MOV  RBX, FunctionAddress
	XOR	 RAX, RAX
	CALL KeGetCurrentProcessorNumber
	MOV  RCX, QWORD PTR[RBX + RAX * 8]
	MOV	 OriginalFunction,RCX

	POP FS

	POP R15
	POP R14
	POP R13
	POP R12
	POP R11
	POP R10
	POP R9
	POP R8
	
	POPF

	POP RDI
	POP RSI
	POP RBP
	POP RSP
	POP RBX
	POP RDX
	POP RCX
	POP RAX
	
	;JMP OriginalFunction ;
	JMP ?oldFunction@@3_KA
HookRoutine1 ENDP

HookRoutine PROC
	;JMP OriginalFunction ;
	JMP ?oldFunction@@3_KA
HookRoutine ENDP
END

;
;VS 64bit C/C++ invoke ASM function,the arguments saved in RCX/RDX/R8/R9, then Push on stack.
;