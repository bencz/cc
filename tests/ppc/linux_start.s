	.text
	.globl _start
	.type _start,@function
_start:
	lwz 3,0(1)
	addi 4,1,4
	slwi 5,3,2
	add 5,4,5
	addi 5,5,4
	bl main
	li 0,1
	sc
	trap
	.size _start,.-_start
	.section .note.GNU-stack,"",@progbits
