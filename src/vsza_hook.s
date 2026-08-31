.intel_syntax noprefix
.section .text
.global _vsza_hook
_vsza_hook:
    cmp dword ptr [esp+8], 23
    jb old_path
    mov eax, dword ptr [esp+4]
    cmp dword ptr [eax], 0x615a5356
    jne old_path

    push ebp
    mov ebp, esp
    sub esp, 12
    push ebx
    push esi
    push edi
    mov esi, dword ptr [ebp+8]
    mov edi, dword ptr [ebp+12]
    mov ebx, dword ptr [ebp+16]

    cmp byte ptr [esi+edi-3], 'z'
    jne fail
    cmp byte ptr [esi+edi-2], 's'
    jne fail
    cmp byte ptr [esi+edi-1], 'v'
    jne fail
    mov eax, dword ptr [esi+edi-11]
    test eax, eax
    jz fail
    cmp eax, dword ptr [ebp+20]
    ja fail
    mov dword ptr [ebp-4], eax
    mov edx, dword ptr [ebx+16]
    mov dword ptr [ebp-8], edx
    add eax, edx
    jc fail
    push eax
    mov ecx, ebx
    .byte 0xe8
    .long 0x38ae4b70 - . - 4
    mov eax, dword ptr [ebx]
    test eax, eax
    jz fail
    add eax, dword ptr [ebp-8]
    mov dword ptr [ebp-12], eax

    call get_pc
get_pc:
    pop edx
    sub edx, 0x5f9c1
    push edx
    lea eax, [edx + dll_name-0x38000000]
    push eax
    call dword ptr [edx + 0xe8a4d4]
    pop edx
    test eax, eax
    jz fail
    lea ecx, [edx + function_name-0x38000000]
    push edx
    push ecx
    push eax
    call dword ptr [edx + 0xe8a4d8]
    pop edx
    test eax, eax
    jz fail
    mov ecx, eax
    push edi
    push esi
    push dword ptr [ebp-4]
    push dword ptr [ebp-12]
    call ecx
    add esp, 16
    test eax, eax
    jz fail
    mov eax, dword ptr [ebp-8]
    add eax, dword ptr [ebp-4]
    mov dword ptr [ebx+16], eax
    mov eax, 1
    jmp done
fail:
    mov eax, 2
done:
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret

old_path:
    push ebp
    mov ebp, esp
    sub esp, 0x28c
    .byte 0xe9
    .long 0x38af8119 - . - 4

dll_name:
    .asciz "steam_zstd.dll"
function_name:
    .asciz "SteamVSZaDecompress"
