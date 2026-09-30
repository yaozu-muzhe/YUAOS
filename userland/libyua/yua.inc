; ============================================================
; YUAOS userland runtime ABI
; 文件 : userland/libyua/yua.inc
;
; runtime 结构体布局（32 位，小端）：
;   偏移  0   print     void (*)(const char*)
;   偏移  4   putc      void (*)(char)
;   偏移  8   getc      int  (*)(void)          ; -1 无键
;   偏移 12   print_hex void (*)(u32)
;   偏移 16   sleep     void (*)(u32 ms)
;   偏移 20   exit      void (*)(int)           ; 不返回
;
; entry 签名（cdecl）：
;   void _start(qa_runtime_t* rt, u8* base)
;     第 1 参数在 [esp+4]
;     第 2 参数在 [esp+8]
; ============================================================

%define RT_PRINT      0
%define RT_PUTC       4
%define RT_GETC       8
%define RT_PRINT_HEX  12
%define RT_SLEEP      16
%define RT_EXIT       20
%define RT_SIZE       24
