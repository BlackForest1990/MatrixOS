ENTRY(loader)

SECTIONS
{
    /* 内核物理加载起始地址：1MB */
    . = 0x00100000;

    /* === Multiboot Header 段 === */
    .multiboot.data : {
        *(.multiboot.data)
    }

    .multiboot.text : {
        *(.multiboot.text)
    }


    /* === 跳转到高半地址虚拟空间 === */
    /* 从这里开始，所有 VMA 都加上 0xC0000000 */
    . += 0xC0000000;

    /* === .text 段：代码 === */
    /* 虚拟地址: 0xC0100000+ */
    /* 物理加载地址: 当前位置 - 0xC0000000 */
    .text ALIGN(4096) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
    }

    /* === .rodata 段：只读数据 === */
    .rodata ALIGN(4096) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    /* === .data 段：初始化数据 === */
    .data ALIGN(4096) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    /* === .bss 段：未初始化数据 === */
    .bss ALIGN(4096) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)  /* 可选：如果你用 bootstrap stack */
    }


    /* === 丢弃不需要的节 === */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.shstrtab)
        *(.symtab)
        *(.strtab)
        *(.debug*)
    }
}

