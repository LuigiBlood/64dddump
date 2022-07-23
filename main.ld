OUTPUT_ARCH (mips)

/* Starts a segment */
#define BEGIN_SEG(name, addr) \
    _##name##SegmentStart = ADDR(.name); \
    _##name##SegmentRomStart = __romPos; \
    .name addr : AT(__romPos)

/* Ends a segment */
#define END_SEG(name) \
    _##name##SegmentEnd = ADDR(.name) + SIZEOF(.name); \
    _##name##SegmentRomEnd = __romPos + SIZEOF(.name); \
    __romPos += SIZEOF(.name);

/* Starts a noload segment which is intended to contain variables which do not have data yet. */
#define BEGIN_NOLOAD(name) \
    _##name##SegmentBssStart = ADDR(.name.noload); \
    .name.noload (NOLOAD) :

/* Ends a noload segment */
#define END_NOLOAD(name) \
    _##name##SegmentBssEnd = ADDR(.name.noload) + SIZEOF(.name.noload); \
    _##name##SegmentBssSize = SIZEOF(.name.noload);

SECTIONS
{
   __romPos = 0;

   BEGIN_SEG(boot, 0x04000000)
   {
      BUILD_DIR/asm/rom_header.o(.text); /* ROM Header */
      BUILD_DIR/boot.6102.o(.data); /* CIC 6102 bootcode */
   }
   END_SEG(boot)

   BEGIN_SEG(code, 0x80000400) SUBALIGN(16)
   {
      BUILD_DIR/asm/entry.o(.start);
      BUILD_DIR/src/main*.o(.text);
      BUILD_DIR/src/process*.o(.text);
      BUILD_DIR/src/ddtextlib*.o(.text);
      BUILD_DIR/src/leo*.o(.text);
      BUILD_DIR/src/ff*.o(.text);

      */libultra_rom.a:*.o(.text);
      */libleo.a:*.o(.text);
      */libnustd.a:*.o(.text);
      */libcart_ultra.a:*.o(.text);
      */libgcc.a:*.o(.text);
      /* data */
      BUILD_DIR/src/main*.o(.data*);
      BUILD_DIR/src/process*.o(.data*);
      BUILD_DIR/src/ddtextlib*.o(.data*);
      BUILD_DIR/src/leo*.o(.data*);
      BUILD_DIR/src/ff*.o(.data*);

      */libultra_rom.a:*.o(.data*);
      */libleo.a:*.o(.data*);
      */libnustd.a:*.o(.data*);
      */libcart_ultra.a:*.o(.data*);

      /* rodata */
      BUILD_DIR/src/main*.o(.*rodata*);
      BUILD_DIR/src/process*.o(.*rodata*);
      BUILD_DIR/src/ddtextlib*.o(.*rodata*);
      BUILD_DIR/src/leo*.o(.*rodata*);
      BUILD_DIR/src/ff*.o(.*rodata*);

      */libultra_rom.a:*.o(.*rodata*);
      */libleo.a:*.o(.*rodata*);
      */libnustd.a:*.o(.*rodata*);
      */libcart_ultra.a:*.o(.*rodata*);
      */libgcc.a:*.o(.*rodata*);
   }
   END_SEG(code)
   BEGIN_NOLOAD(code)
   {
      BUILD_DIR/src/main*.o(.*bss*);
      BUILD_DIR/src/process*.o(.*bss*);
      BUILD_DIR/src/ddtextlib*.o(.*bss*);
      BUILD_DIR/src/leo*.o(.*bss*);
      BUILD_DIR/src/ff*.o(.*bss*);

      */libultra_rom.a:*.o(COMMON);
      */libultra_rom.a:*.o(.scommon);
      */libultra_rom.a:*.o(.*bss*);
      */libleo.a:*.o(COMMON);
      */libleo.a:*.o(.scommon);
      */libleo.a:*.o(.*bss*);
      */libnustd.a:*.o(COMMON);
      */libnustd.a:*.o(.scommon);
      */libnustd.a:*.o(.*bss*);
      */libcart_ultra.a:*.o(COMMON);
      */libcart_ultra.a:*.o(.scommon);
      */libcart_ultra.a:*.o(.*bss*);
      . = ALIGN(0x40);
   }
   END_NOLOAD(code)
   
   BEGIN_NOLOAD(cfb) SUBALIGN(64)
   {
      . = ALIGN(0x40);
      BUILD_DIR/src/buffers/cfb.o(.*bss*);
   }
   END_NOLOAD(cfb)

   BEGIN_SEG(ddfont, __romPos)
   {
      BUILD_DIR/data/ddfont.o(.*rodata*);
   }
   END_SEG(ddfont)

   /* Discard everything not specifically mentioned above. */
   /DISCARD/ :
   {
      *(*);
   }
}
