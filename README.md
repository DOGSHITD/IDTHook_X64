# IDTHook_X64
Hook IDT vector 0xb2 to detect SCI in 64bit windows. 

/*
Code by BoyceHong.
Time at 2016.07.19.
Any problem cantact solidman@msn.cn
*/

相关说明：
  首先需要声明的是目前功能可以实现，但是会有蓝屏，不稳定，还需后续完善。
  
    网上已经有资料描述在32位系统怎么去Hook IDT，但是微软已经禁止在64位系统Hook 
    IDT，所以网上相关完整资料几乎没有，即使翻墙google查找也没有。64位Hook实现的
    难处较多，比如64位VC++不允许直接嵌入汇编，不许有naked等属性声明，汇编里FS寄
    存器也不再是Ring0/Ring3切换等。
    
    本实例由三层组成，底层是Windows driver，Hook idt就在驱动里操作,通过Hook 0xB2
    号中断来实现截取ACPI SCI，第二层是C++写的关于Driver管理的动态库，供上层调用，
    最上层是C#写的简单界面。
    
    我本身是做Bios的，对OS下软件编程其实不了解，也是边学边做，希望一起讨论实现。
    其他bios相关问题也可以一起讨论哇 0.0 .
