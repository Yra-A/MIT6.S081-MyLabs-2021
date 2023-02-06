	# qemu -kernel loads the kernel at 0x80000000
        # and causes each CPU to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
	# set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0 // sp 寄存器保存栈的地址, stack0 是每个 CPU 上的栈的起始地址
        li a0, 1024*4 // a0 保存栈大小？
	csrr a1, mhartid // 把当前 CPU 的 ID 读到 a1 寄存器中
        addi a1, a1, 1 // a1 = a1 + 1
        mul a0, a0, a1 // a0 = a0 * a1 <--> hartid * 4096
        add sp, sp, a0 // sp = stack0 + (hartid * 4096)
	# jump to start() in start.c
        call start
spin:  //如果返回就死循环
        j spin
