.arch armv7a

.section .vitalink.fstubs,"ax",%progbits

	.align 4
	.global scePromoterUtilityInit
	.type scePromoterUtilityInit, %function
scePromoterUtilityInit:
	.word 0xB31C2D5C
	.word 0x31F237B6
	.word 0x93451536
	.align 4

