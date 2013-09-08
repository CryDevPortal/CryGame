; Compile with full debug information
; 	ml64.exe /Zi /c /Flcpuid64.lst /Focpuid64.obj cpuid64.asm

; Compile for release
; 	ml64.exe /c cpuid64.asm

; call cpuid with args in eax, ecx
; store eax, ebx, ecx, edx to p

public cpuid64

.code
       align    8

cpuid64	proc frame

; void cpuid64(int* p);
; rcx <= p

        sub			rsp, 32
        .allocstack 32
        push		rbx
        .pushreg	rbx
        .endprolog
        
				mov	r8, rcx
				mov eax, dword ptr [r8+0]
				mov ecx, dword ptr [r8+8]
				cpuid
				mov dword ptr [r8+0], eax
				mov dword ptr [r8+4], ebx
				mov dword ptr [r8+8], ecx
				mov dword ptr [r8+12], edx

        pop      rbx         
        add      rsp, 32     
        
        ret                  
        align    8

cpuid64 endp

_text ends

end