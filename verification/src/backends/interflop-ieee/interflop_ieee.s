	.file	"interflop_ieee.c"
	.text
	.section	.rodata
.LC0:
	.string	" x 2^%d"
	.text
	.globl	double_to_binary
	.type	double_to_binary, @function
double_to_binary:
.LFB5:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$160, %rsp
	movsd	%xmm0, -152(%rbp)
	movq	%rdi, -160(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$0, -128(%rbp)
	movl	$52, -124(%rbp)
	movsd	-152(%rbp), %xmm0
	movsd	%xmm0, -120(%rbp)
	movzbl	-113(%rbp), %eax
	shrb	$7, %al
	movb	%al, -141(%rbp)
	movzwl	-114(%rbp), %eax
	shrw	$4, %ax
	andw	$2047, %ax
	movw	%ax, -138(%rbp)
	movl	-116(%rbp), %eax
	andl	$1048575, %eax
	movl	%eax, %eax
	movq	%rax, -104(%rbp)
	salq	$32, -104(%rbp)
	movl	-120(%rbp), %eax
	movl	%eax, %eax
	orq	%rax, -104(%rbp)
	movl	$0, -132(%rbp)
	cmpb	$1, -141(%rbp)
	jne	.L2
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$45, (%rax)
.L2:
	cmpw	$0, -138(%rbp)
	jne	.L3
	cmpq	$0, -104(%rbp)
	jne	.L3
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$48, (%rax)
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$0, (%rax)
	jmp	.L4
.L3:
	cmpw	$0, -138(%rbp)
	jne	.L5
	cmpq	$0, -104(%rbp)
	je	.L5
	movq	-104(%rbp), %rax
	movq	%rax, -112(%rbp)
	movw	$-1022, -140(%rbp)
	jmp	.L6
.L7:
	movzwl	-140(%rbp), %eax
	subl	$1, %eax
	movw	%ax, -140(%rbp)
	addl	$1, -128(%rbp)
.L6:
	movl	$52, %eax
	subl	-128(%rbp), %eax
	movq	-112(%rbp), %rdx
	movl	%eax, %ecx
	shrq	%cl, %rdx
	movq	%rdx, %rax
	andl	$1, %eax
	testq	%rax, %rax
	je	.L7
	jmp	.L8
.L5:
	movabsq	$4503599627370496, %rax
	orq	-104(%rbp), %rax
	movq	%rax, -112(%rbp)
	movzwl	-138(%rbp), %eax
	subw	$1023, %ax
	movw	%ax, -140(%rbp)
.L8:
	jmp	.L9
.L10:
	subl	$1, -124(%rbp)
.L9:
	movl	$52, %eax
	subl	-124(%rbp), %eax
	movq	-112(%rbp), %rdx
	movl	%eax, %ecx
	shrq	%cl, %rdx
	movq	%rdx, %rax
	andl	$1, %eax
	testq	%rax, %rax
	je	.L10
	movl	-128(%rbp), %eax
	movl	%eax, -136(%rbp)
	jmp	.L11
.L15:
	movl	$52, %eax
	subl	-136(%rbp), %eax
	movq	-112(%rbp), %rdx
	movl	%eax, %ecx
	shrq	%cl, %rdx
	movq	%rdx, %rax
	andl	$1, %eax
	testq	%rax, %rax
	je	.L12
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$49, (%rax)
	jmp	.L13
.L12:
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$48, (%rax)
.L13:
	movl	-136(%rbp), %eax
	cmpl	-128(%rbp), %eax
	jne	.L14
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$46, (%rax)
.L14:
	addl	$1, -136(%rbp)
.L11:
	movl	-136(%rbp), %eax
	cmpl	-124(%rbp), %eax
	jle	.L15
	movl	-128(%rbp), %eax
	cmpl	-124(%rbp), %eax
	jne	.L16
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$48, (%rax)
.L16:
	movl	-132(%rbp), %eax
	leal	1(%rax), %edx
	movl	%edx, -132(%rbp)
	movslq	%eax, %rdx
	movq	-160(%rbp), %rax
	addq	%rdx, %rax
	movb	$0, (%rax)
	movswl	-140(%rbp), %edx
	leaq	-96(%rbp), %rax
	leaq	.LC0(%rip), %rsi
	movq	%rax, %rdi
	movl	$0, %eax
	call	sprintf@PLT
	leaq	-96(%rbp), %rdx
	movq	-160(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	strcat@PLT
.L4:
	nop
	movq	-8(%rbp), %rax
	xorq	%fs:40, %rax
	je	.L17
	call	__stack_chk_fail@PLT
.L17:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE5:
	.size	double_to_binary, .-double_to_binary
	.section	.rodata
.LC1:
	.string	"+"
	.text
	.type	_interflop_add_float, @function
_interflop_add_float:
.LFB7:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movss	%xmm0, -4(%rbp)
	movss	%xmm1, -8(%rbp)
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movss	-4(%rbp), %xmm0
	addss	-8(%rbp), %xmm0
	movq	-16(%rbp), %rax
	movss	%xmm0, (%rax)
	movq	-16(%rbp), %rax
	movss	(%rax), %xmm0
	cvtss2sd	%xmm0, %xmm2
	cvtss2sd	-8(%rbp), %xmm1
	cvtss2sd	-4(%rbp), %xmm0
	movq	-24(%rbp), %rax
	leaq	.LC1(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	_interflop_add_float, .-_interflop_add_float
	.section	.rodata
.LC2:
	.string	"-"
	.text
	.type	_interflop_sub_float, @function
_interflop_sub_float:
.LFB8:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movss	%xmm0, -4(%rbp)
	movss	%xmm1, -8(%rbp)
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movss	-4(%rbp), %xmm0
	subss	-8(%rbp), %xmm0
	movq	-16(%rbp), %rax
	movss	%xmm0, (%rax)
	movq	-16(%rbp), %rax
	movss	(%rax), %xmm0
	cvtss2sd	%xmm0, %xmm2
	cvtss2sd	-8(%rbp), %xmm1
	cvtss2sd	-4(%rbp), %xmm0
	movq	-24(%rbp), %rax
	leaq	.LC2(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	_interflop_sub_float, .-_interflop_sub_float
	.section	.rodata
.LC3:
	.string	"*"
	.text
	.type	_interflop_mul_float, @function
_interflop_mul_float:
.LFB9:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movss	%xmm0, -4(%rbp)
	movss	%xmm1, -8(%rbp)
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movss	-4(%rbp), %xmm0
	mulss	-8(%rbp), %xmm0
	movq	-16(%rbp), %rax
	movss	%xmm0, (%rax)
	movq	-16(%rbp), %rax
	movss	(%rax), %xmm0
	cvtss2sd	%xmm0, %xmm2
	cvtss2sd	-8(%rbp), %xmm1
	cvtss2sd	-4(%rbp), %xmm0
	movq	-24(%rbp), %rax
	leaq	.LC3(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE9:
	.size	_interflop_mul_float, .-_interflop_mul_float
	.section	.rodata
.LC4:
	.string	"/"
	.text
	.type	_interflop_div_float, @function
_interflop_div_float:
.LFB10:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movss	%xmm0, -4(%rbp)
	movss	%xmm1, -8(%rbp)
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movss	-4(%rbp), %xmm0
	divss	-8(%rbp), %xmm0
	movq	-16(%rbp), %rax
	movss	%xmm0, (%rax)
	movq	-16(%rbp), %rax
	movss	(%rax), %xmm0
	cvtss2sd	%xmm0, %xmm2
	cvtss2sd	-8(%rbp), %xmm1
	cvtss2sd	-4(%rbp), %xmm0
	movq	-24(%rbp), %rax
	leaq	.LC4(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	_interflop_div_float, .-_interflop_div_float
	.section	.rodata
.LC5:
	.string	"FCMP_FALSE"
.LC6:
	.string	"FCMP_OEQ"
.LC7:
	.string	"FCMP_OGT"
.LC8:
	.string	"FCMP_OGE"
.LC9:
	.string	"FCMP_OLT"
.LC10:
	.string	"FCMP_OLE"
.LC11:
	.string	"FCMP_ONE"
.LC12:
	.string	"FCMP_ORD"
.LC13:
	.string	"FCMP_UEQ"
.LC14:
	.string	""
.LC15:
	.string	"FCMP_UGT"
.LC16:
	.string	"FCMP_ULT"
.LC17:
	.string	"FCMP_ULE"
.LC18:
	.string	"FCMP_UNE"
.LC19:
	.string	"FCMP_UNO"
.LC20:
	.string	"FCMP_TRUE"
	.text
	.type	_interflop_cmp_float, @function
_interflop_cmp_float:
.LFB11:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movl	%edi, -20(%rbp)
	movss	%xmm0, -24(%rbp)
	movss	%xmm1, -28(%rbp)
	movq	%rsi, -40(%rbp)
	movq	%rdx, -48(%rbp)
	cmpl	$15, -20(%rbp)
	ja	.L23
	movl	-20(%rbp), %eax
	leaq	0(,%rax,4), %rdx
	leaq	.L25(%rip), %rax
	movl	(%rdx,%rax), %eax
	movslq	%eax, %rdx
	leaq	.L25(%rip), %rax
	addq	%rdx, %rax
	jmp	*%rax
	.section	.rodata
	.align 4
	.align 4
.L25:
	.long	.L24-.L25
	.long	.L26-.L25
	.long	.L27-.L25
	.long	.L28-.L25
	.long	.L29-.L25
	.long	.L30-.L25
	.long	.L31-.L25
	.long	.L32-.L25
	.long	.L33-.L25
	.long	.L34-.L25
	.long	.L35-.L25
	.long	.L36-.L25
	.long	.L37-.L25
	.long	.L38-.L25
	.long	.L39-.L25
	.long	.L40-.L25
	.text
.L24:
	movq	-40(%rbp), %rax
	movl	$0, (%rax)
	leaq	.LC5(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L26:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L41
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L41
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L41
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jne	.L41
	movl	$1, %edx
	jmp	.L43
.L41:
	movl	$0, %edx
.L43:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC6(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L27:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L44
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L44
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jbe	.L44
	movl	$1, %edx
	jmp	.L46
.L44:
	movl	$0, %edx
.L46:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC7(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L28:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L47
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L47
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jb	.L47
	movl	$1, %edx
	jmp	.L49
.L47:
	movl	$0, %edx
.L49:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC8(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L29:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L50
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L50
	movss	-28(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jbe	.L50
	movl	$1, %edx
	jmp	.L52
.L50:
	movl	$0, %edx
.L52:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC9(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L30:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L53
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L53
	movss	-28(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jb	.L53
	movl	$1, %edx
	jmp	.L55
.L53:
	movl	$0, %edx
.L55:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC10(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L31:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L56
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L56
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L93
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	je	.L56
.L93:
	movl	$1, %edx
	jmp	.L58
.L56:
	movl	$0, %edx
.L58:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC11(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L32:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jp	.L59
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L59
	movl	$1, %edx
	jmp	.L60
.L59:
	movl	$0, %edx
.L60:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC12(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L34:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L61
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jnp	.L61
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L62
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jne	.L62
.L61:
	movl	$1, %edx
	jmp	.L64
.L62:
	movl	$0, %edx
.L64:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC13(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L35:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L65
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jnp	.L65
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jbe	.L94
.L65:
	movl	$1, %edx
	jmp	.L68
.L94:
	movl	$0, %edx
.L68:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC14(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L36:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L69
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jnp	.L69
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jb	.L95
.L69:
	movl	$1, %edx
	jmp	.L72
.L95:
	movl	$0, %edx
.L72:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC15(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L37:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L73
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jnp	.L73
	movss	-28(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jbe	.L96
.L73:
	movl	$1, %edx
	jmp	.L76
.L96:
	movl	$0, %edx
.L76:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC16(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L38:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L77
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jnp	.L77
	movss	-28(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jb	.L97
.L77:
	movl	$1, %edx
	jmp	.L80
.L97:
	movl	$0, %edx
.L80:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC17(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L39:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L81
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jnp	.L81
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L81
	movss	-24(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	je	.L98
.L81:
	movl	$1, %edx
	jmp	.L84
.L98:
	movl	$0, %edx
.L84:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC18(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L33:
	movss	-24(%rbp), %xmm0
	ucomiss	-24(%rbp), %xmm0
	jnp	.L85
	movss	-28(%rbp), %xmm0
	ucomiss	-28(%rbp), %xmm0
	jp	.L86
.L85:
	movl	$1, %edx
	jmp	.L87
.L86:
	movl	$0, %edx
.L87:
	movq	-40(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC19(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L23
.L40:
	movq	-40(%rbp), %rax
	movl	$1, (%rax)
	leaq	.LC20(%rip), %rax
	movq	%rax, -8(%rbp)
	nop
.L23:
	movq	-40(%rbp), %rax
	movl	(%rax), %eax
	cvtsi2sd	%eax, %xmm2
	cvtss2sd	-28(%rbp), %xmm1
	cvtss2sd	-24(%rbp), %xmm0
	movq	-8(%rbp), %rdx
	movq	-48(%rbp), %rax
	movl	$1, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE11:
	.size	_interflop_cmp_float, .-_interflop_cmp_float
	.type	_interflop_add_double, @function
_interflop_add_double:
.LFB12:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movsd	%xmm0, -8(%rbp)
	movsd	%xmm1, -16(%rbp)
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movsd	-8(%rbp), %xmm0
	addsd	-16(%rbp), %xmm0
	movq	-24(%rbp), %rax
	movsd	%xmm0, (%rax)
	movq	-24(%rbp), %rax
	movsd	(%rax), %xmm1
	movsd	-16(%rbp), %xmm0
	movq	-8(%rbp), %rdx
	movq	-32(%rbp), %rax
	movapd	%xmm1, %xmm2
	movapd	%xmm0, %xmm1
	movq	%rdx, -40(%rbp)
	movsd	-40(%rbp), %xmm0
	leaq	.LC1(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE12:
	.size	_interflop_add_double, .-_interflop_add_double
	.type	_interflop_sub_double, @function
_interflop_sub_double:
.LFB13:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movsd	%xmm0, -8(%rbp)
	movsd	%xmm1, -16(%rbp)
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movsd	-8(%rbp), %xmm0
	subsd	-16(%rbp), %xmm0
	movq	-24(%rbp), %rax
	movsd	%xmm0, (%rax)
	movq	-24(%rbp), %rax
	movsd	(%rax), %xmm1
	movsd	-16(%rbp), %xmm0
	movq	-8(%rbp), %rdx
	movq	-32(%rbp), %rax
	movapd	%xmm1, %xmm2
	movapd	%xmm0, %xmm1
	movq	%rdx, -40(%rbp)
	movsd	-40(%rbp), %xmm0
	leaq	.LC2(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE13:
	.size	_interflop_sub_double, .-_interflop_sub_double
	.type	_interflop_mul_double, @function
_interflop_mul_double:
.LFB14:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movsd	%xmm0, -8(%rbp)
	movsd	%xmm1, -16(%rbp)
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movsd	-8(%rbp), %xmm0
	mulsd	-16(%rbp), %xmm0
	movq	-24(%rbp), %rax
	movsd	%xmm0, (%rax)
	movq	-24(%rbp), %rax
	movsd	(%rax), %xmm1
	movsd	-16(%rbp), %xmm0
	movq	-8(%rbp), %rdx
	movq	-32(%rbp), %rax
	movapd	%xmm1, %xmm2
	movapd	%xmm0, %xmm1
	movq	%rdx, -40(%rbp)
	movsd	-40(%rbp), %xmm0
	leaq	.LC3(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE14:
	.size	_interflop_mul_double, .-_interflop_mul_double
	.type	_interflop_div_double, @function
_interflop_div_double:
.LFB15:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movsd	%xmm0, -8(%rbp)
	movsd	%xmm1, -16(%rbp)
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movsd	-8(%rbp), %xmm0
	divsd	-16(%rbp), %xmm0
	movq	-24(%rbp), %rax
	movsd	%xmm0, (%rax)
	movq	-24(%rbp), %rax
	movsd	(%rax), %xmm1
	movsd	-16(%rbp), %xmm0
	movq	-8(%rbp), %rdx
	movq	-32(%rbp), %rax
	movapd	%xmm1, %xmm2
	movapd	%xmm0, %xmm1
	movq	%rdx, -40(%rbp)
	movsd	-40(%rbp), %xmm0
	leaq	.LC4(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
	.size	_interflop_div_double, .-_interflop_div_double
	.type	_interflop_cmp_double, @function
_interflop_cmp_double:
.LFB16:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$64, %rsp
	movl	%edi, -20(%rbp)
	movsd	%xmm0, -32(%rbp)
	movsd	%xmm1, -40(%rbp)
	movq	%rsi, -48(%rbp)
	movq	%rdx, -56(%rbp)
	cmpl	$15, -20(%rbp)
	ja	.L104
	movl	-20(%rbp), %eax
	leaq	0(,%rax,4), %rdx
	leaq	.L106(%rip), %rax
	movl	(%rdx,%rax), %eax
	movslq	%eax, %rdx
	leaq	.L106(%rip), %rax
	addq	%rdx, %rax
	jmp	*%rax
	.section	.rodata
	.align 4
	.align 4
.L106:
	.long	.L105-.L106
	.long	.L107-.L106
	.long	.L108-.L106
	.long	.L109-.L106
	.long	.L110-.L106
	.long	.L111-.L106
	.long	.L112-.L106
	.long	.L113-.L106
	.long	.L114-.L106
	.long	.L115-.L106
	.long	.L116-.L106
	.long	.L117-.L106
	.long	.L118-.L106
	.long	.L119-.L106
	.long	.L120-.L106
	.long	.L121-.L106
	.text
.L105:
	movq	-48(%rbp), %rax
	movl	$0, (%rax)
	leaq	.LC5(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L107:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L122
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L122
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L122
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jne	.L122
	movl	$1, %edx
	jmp	.L124
.L122:
	movl	$0, %edx
.L124:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC6(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L108:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L125
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L125
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jbe	.L125
	movl	$1, %edx
	jmp	.L127
.L125:
	movl	$0, %edx
.L127:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC7(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L109:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L128
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L128
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jb	.L128
	movl	$1, %edx
	jmp	.L130
.L128:
	movl	$0, %edx
.L130:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC8(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L110:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L131
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L131
	movsd	-40(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jbe	.L131
	movl	$1, %edx
	jmp	.L133
.L131:
	movl	$0, %edx
.L133:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC9(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L111:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L134
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L134
	movsd	-40(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jb	.L134
	movl	$1, %edx
	jmp	.L136
.L134:
	movl	$0, %edx
.L136:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC10(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L112:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L137
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L137
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L174
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	je	.L137
.L174:
	movl	$1, %edx
	jmp	.L139
.L137:
	movl	$0, %edx
.L139:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC11(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L113:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jp	.L140
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L140
	movl	$1, %edx
	jmp	.L141
.L140:
	movl	$0, %edx
.L141:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC12(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L115:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L142
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jnp	.L142
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L143
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jne	.L143
.L142:
	movl	$1, %edx
	jmp	.L145
.L143:
	movl	$0, %edx
.L145:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC13(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L116:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L146
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jnp	.L146
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jbe	.L175
.L146:
	movl	$1, %edx
	jmp	.L149
.L175:
	movl	$0, %edx
.L149:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC14(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L117:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L150
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jnp	.L150
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jb	.L176
.L150:
	movl	$1, %edx
	jmp	.L153
.L176:
	movl	$0, %edx
.L153:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC15(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L118:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L154
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jnp	.L154
	movsd	-40(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jbe	.L177
.L154:
	movl	$1, %edx
	jmp	.L157
.L177:
	movl	$0, %edx
.L157:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC16(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L119:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L158
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jnp	.L158
	movsd	-40(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jb	.L178
.L158:
	movl	$1, %edx
	jmp	.L161
.L178:
	movl	$0, %edx
.L161:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC17(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L120:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L162
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jnp	.L162
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L162
	movsd	-32(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	je	.L179
.L162:
	movl	$1, %edx
	jmp	.L165
.L179:
	movl	$0, %edx
.L165:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC18(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L114:
	movsd	-32(%rbp), %xmm0
	ucomisd	-32(%rbp), %xmm0
	jnp	.L166
	movsd	-40(%rbp), %xmm0
	ucomisd	-40(%rbp), %xmm0
	jp	.L167
.L166:
	movl	$1, %edx
	jmp	.L168
.L167:
	movl	$0, %edx
.L168:
	movq	-48(%rbp), %rax
	movl	%edx, (%rax)
	leaq	.LC19(%rip), %rax
	movq	%rax, -8(%rbp)
	jmp	.L104
.L121:
	movq	-48(%rbp), %rax
	movl	$1, (%rax)
	leaq	.LC20(%rip), %rax
	movq	%rax, -8(%rbp)
	nop
.L104:
	movq	-48(%rbp), %rax
	movl	(%rax), %eax
	cvtsi2sd	%eax, %xmm0
	movsd	-40(%rbp), %xmm1
	movq	-32(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	-56(%rbp), %rax
	movapd	%xmm0, %xmm2
	movq	%rcx, -64(%rbp)
	movsd	-64(%rbp), %xmm0
	movl	$1, %esi
	movq	%rax, %rdi
	call	debug_print_flt@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	_interflop_cmp_double, .-_interflop_cmp_double
	.section	.rodata
.LC21:
	.string	"debug"
.LC22:
	.string	"enable debug output"
.LC23:
	.string	"debug_binary"
.LC24:
	.string	"enable binary debug output"
	.section	.data.rel.local,"aw",@progbits
	.align 32
	.type	options, @object
	.size	options, 144
options:
	.quad	.LC21
	.long	100
	.zero	4
	.quad	0
	.long	0
	.zero	4
	.quad	.LC22
	.zero	8
	.quad	.LC23
	.long	98
	.zero	4
	.quad	0
	.long	0
	.zero	4
	.quad	.LC24
	.zero	8
	.quad	0
	.zero	40
	.text
	.type	parse_opt, @function
parse_opt:
.LFB17:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	movq	%rdx, -40(%rbp)
	movq	-40(%rbp), %rax
	movq	40(%rax), %rax
	movq	%rax, -8(%rbp)
	movl	-20(%rbp), %eax
	cmpl	$98, %eax
	je	.L182
	cmpl	$100, %eax
	jne	.L186
	movq	-8(%rbp), %rax
	movl	$1, (%rax)
	jmp	.L184
.L182:
	movq	-8(%rbp), %rax
	movl	$1, 4(%rax)
	jmp	.L184
.L186:
	movl	$7, %eax
	jmp	.L180
.L184:
.L180:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE17:
	.size	parse_opt, .-parse_opt
	.section	.data.rel.local
	.align 32
	.type	argp, @object
	.size	argp, 56
argp:
	.quad	options
	.quad	parse_opt
	.quad	.LC14
	.quad	.LC14
	.zero	24
	.text
	.globl	interflop_init
	.type	interflop_init, @function
interflop_init:
.LFB18:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	addq	$-128, %rsp
	movq	%rdi, -104(%rbp)
	movl	%esi, -108(%rbp)
	movq	%rdx, -120(%rbp)
	movq	%rcx, -128(%rbp)
	movl	$8, %esi
	movl	$1, %edi
	call	calloc@PLT
	movq	%rax, -88(%rbp)
	movq	-88(%rbp), %rcx
	movq	-120(%rbp), %rdx
	movl	-108(%rbp), %eax
	movq	%rcx, %r9
	movl	$0, %r8d
	movl	$0, %ecx
	movl	%eax, %esi
	leaq	argp(%rip), %rdi
	call	argp_parse@PLT
	movq	-128(%rbp), %rax
	movq	-88(%rbp), %rdx
	movq	%rdx, (%rax)
	leaq	_interflop_add_float(%rip), %rax
	movq	%rax, -80(%rbp)
	leaq	_interflop_sub_float(%rip), %rax
	movq	%rax, -72(%rbp)
	leaq	_interflop_mul_float(%rip), %rax
	movq	%rax, -64(%rbp)
	leaq	_interflop_div_float(%rip), %rax
	movq	%rax, -56(%rbp)
	leaq	_interflop_cmp_float(%rip), %rax
	movq	%rax, -48(%rbp)
	leaq	_interflop_add_double(%rip), %rax
	movq	%rax, -40(%rbp)
	leaq	_interflop_sub_double(%rip), %rax
	movq	%rax, -32(%rbp)
	leaq	_interflop_mul_double(%rip), %rax
	movq	%rax, -24(%rbp)
	leaq	_interflop_div_double(%rip), %rax
	movq	%rax, -16(%rbp)
	leaq	_interflop_cmp_double(%rip), %rax
	movq	%rax, -8(%rbp)
	movq	-104(%rbp), %rax
	movq	-80(%rbp), %rdx
	movq	-72(%rbp), %rcx
	movq	%rdx, (%rax)
	movq	%rcx, 8(%rax)
	movq	-64(%rbp), %rdx
	movq	-56(%rbp), %rcx
	movq	%rdx, 16(%rax)
	movq	%rcx, 24(%rax)
	movq	-48(%rbp), %rdx
	movq	-40(%rbp), %rcx
	movq	%rdx, 32(%rax)
	movq	%rcx, 40(%rax)
	movq	-32(%rbp), %rdx
	movq	-24(%rbp), %rcx
	movq	%rdx, 48(%rax)
	movq	%rcx, 56(%rax)
	movq	-16(%rbp), %rdx
	movq	-8(%rbp), %rcx
	movq	%rdx, 64(%rax)
	movq	%rcx, 72(%rax)
	movq	-104(%rbp), %rax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE18:
	.size	interflop_init, .-interflop_init
	.ident	"GCC: (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0"
	.section	.note.GNU-stack,"",@progbits
