.arch armv7a

.section .vitalink.fstubs,"ax",%progbits

	.align 4
	.global scePromoterUtilityExit
	.type scePromoterUtilityExit, %function
scePromoterUtilityExit:
	.word 0xB31C2D5C
	.word 0x31F237B6
	.word 0xC95D24A6
	.align 4

