#	Vitamin
#	Copyright (C) 2016, Team FreeK
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	.section .text
	.thumb

	.global payload_start
payload_start:
	# No operation to support the pc relativity
	nop

	# Push stack
	sub sp, #0x10

	# Save arguments
	strd a1, a2, [sp, #8]

	# Set fith and sixth argument to zero in stack
	mov a1, #0
	strd a1, a1, [sp]

	# Load from 0x11111111 to a1 (path)
	ldr a1, [pc, #0x20]

	# Set second, third and forth argument to zero
	mov a2, #0
	mov a3, #0
	mov a4, #0

	# Execute 0x22222222 in arm mode (sceKernelLoadStartModule)
	ldr v1, [pc, #0x1C]
	blx v1

	# Restore arguments
	ldrd a1, a2, [sp, #8]

	# Pop stack
	add sp, #0x10

	# Execute 0x33333333 in thumb mode (module_start)
	ldr v1, [pc, #0x14]
	bx v1

	# PC relative pointers
	.align 4
	.word 0x11111111
	.word 0x22222222
	.word 0x33333333
	.word 0x44444444

	.global payload_end
payload_end:
