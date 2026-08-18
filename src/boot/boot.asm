; Copyright (C) 2026 Vlad944467
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <https://www.gnu.org/licenses/>.

global _start
extern main

MBALIGN   equ 1 << 0
MEMINFO   equ 1 << 1
FLAGS     equ MBALIGN | MEMINFO
MAGIC     equ 0x1BADB002
CHECKSUM  equ -(MAGIC + FLAGS)

section .multiboot
    align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
_start:
    mov esp, stack_top
    call main
    cli
hang:
    hlt
    jmp hang

section .bss
    align 16
stack_bottom:
    resb 16384
stack_top:
