/*
** ###################################################################
**     Processors:          K32W041-061
**
**     Compiler:            GNU C Compiler
**     Reference manual:    K32W0x1 Series Reference Manual
**
**     Abstract:
**         Linker file for the GNU C Compiler
**
**     Copyright  2019 NXP
**     All rights reserved.
**
**     Abstract:
**         Linker file for the GNU C Compiler
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2018-2019 NXP
**     All rights reserved.
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
** ###################################################################
*/

/******************* Map of K32W061 FLASH ***********************************


             0x000a_0000    m_flash_end
    - - - +---------------+ - - - - - - - -
          |   _ _ _ _ _   |
          | Flash config  |
   8.5k   |   - - - - -   |
          |               |
          |  RESERVED     |  0x9de00
    - - - +---------------+ - - - - - - - -
          |               | NV_STORAGE_START_ADDRESS
   31.5k  |               |
          |   NVM_region  |
          |               |
          |               | NV_STORAGE_END_ADDRESS
    - - - +---------------+ - - - - - - - -
          |               | m_app2_end
          |               |
          |               |
   ~241k  |  Application2 |
          |               |
          |               |
          |               |
          |               | m_app2_start = 0x53000
    - - - +---------------+ - - - - - - - -
          |               | m_app1_end
          |               |
          |               |
  ~323k*  |  Application1 |
          |               |
          |               |
          |               | m_app1_start
    - - - +---------------+ - - - - - - - -
          |               | m_OTA_update_end
          |               |
   8k     |  OTA update   |
          |               |
          |               | m_OTA_update_start = m_SSBL_end
    - - - +---------------+ - - - - - - - -
          |               | m_SSBL_end
          |               |
   8k     |  Bootloader*  |
          |               |
          |               | m_SSBL_start = m_flash_start
    - - - +---------------+ - - - - - - - -
             0x0000_0000

 * - Application will occupy all the free space available
 *****************************************************************************/

/* Entry Point */
ENTRY(ResetISR)

/*** flash memory characteristics ***/
m_flash_start   = 0x00000000;
m_flash_end     = 0x0009FFFF;
m_flash_size    = 0x000A0000;
m_sector_size   = 512;
m_qspi_flash_start = 0x10000000;
m_flash_res_sect = 17; /* number of reserved sectors in flash */

/****************************************************
 *******         User Defines                 *******
 ****************************************************/
UseNVMLink              = DEFINED(gUseNVMLink_d)            ? gUseNVMLink_d             : 0;
NVMSectorCountLink      = DEFINED(gNVMSectorCountLink_d)    ? gNVMSectorCountLink_d     : ( DEFINED(gUseNVMLink_d) ? gUseNVMLink_d*63 : 0 );
ram_vector_table        = DEFINED(__ram_vector_table__)     ? __ram_vector_table__      : 0;
UseInternalStorageLink  = DEFINED(gUseInternalStorageLink_d)? gUseInternalStorageLink_d : 0;
m_SSBL_size             = DEFINED(__ssbl_size__)            ? __ssbl_size__             : 0x2000;
m_app1_size             = DEFINED(__app1_size__)            ? __app1_size__             : 0x4f000;
m_app2_size             = DEFINED(__app2_size__)            ? __app2_size__             : 0x40000;
STACK_SIZE              = DEFINED(__stack_size__)           ? __stack_size__            : (0x1000-32);
vector_table_size       = 0x120;

ExtraMACTxBufs          = DEFINED(ExtraMACTxBufs)           ? ExtraMACTxBufs            : 0;
MACHeapSize             = ExtraMACTxBufs * 204;
/* 0 below must be set to 2 if PDM debug version is used */
PDMHeapSize             = (12 + 0) * NVMSectorCountLink;
HEAP_SIZE               = DEFINED(__heap_size__) ? __heap_size__ : PDMHeapSize + ExtraMACTxBufs;

ASSERT(HEAP_SIZE >= PDMHeapSize + ExtraMACTxBufs, "Wrong HEAP size setting")

/*
 * stack size for the boot rom during warm boot and application
 * 256 is sufficient (pwrm_test) but keep it large to 1024
 */
BOOT_RESUME_STACK_SIZE = DEFINED(__boot_resume_stack) ? __boot_resume_stack__ : 1024;


/****************************************************
 ******          Flash regions sizes           ******
 ****************************************************/
m_OTA_update_size  = m_SSBL_size;

NV_STORAGE_SIZE         = NVMSectorCountLink * m_sector_size;
NV_STORAGE_MAX_SECTORS  = NVMSectorCountLink;
NV_STORAGE_SECTOR_SIZE = m_sector_size;

/****************************************************
 ******        Flash Regions, low to high      ******
 ****************************************************/
m_SSBL_start              = m_flash_start;
m_OTA_update_start        = m_SSBL_size;
m_OTA_update_end          = m_OTA_update_start + m_OTA_update_size - 1;

m_app1_start              = m_OTA_update_end + 1;
m_app1_end                = m_app1_start + m_app1_size - 1;

m_app2_start              = m_app1_end + 1;
m_app2_end                = m_app2_start + m_app2_size - 1;

m_app_size          = DEFINED(__app_stated_size__) ? __app_stated_size__ : DEFINED(__app_id__) ? ((__app_id__==0) ? m_SSBL_size :  (__app_id__==1) ? m_app1_size : (__app_id__ ==2) ? m_app2_size : 0x0) : 0x48000;



m_app_start               = DEFINED(__app_load_address__) ? __app_load_address__ : DEFINED(__app_id__)&&(__app_id__== 0) ? m_SSBL_start : DEFINED(__app_id__)&&(__app_id__==1) ? m_app1_start : DEFINED(__app_id__) && (__app_id__ ==2) ? m_app2_start : 0x0;

m_app_end                 = m_app_start + m_app_size - 1;


m_interrupts_start        = m_app_start;
m_interrupts_end          = m_interrupts_start + vector_table_size - 1;

m_int_start_addr          = m_app2_end + 1;

/****************************************************
 *****        Flash Regions, high to low       ******
 ****************************************************/
NV_STORAGE_START_ADDRESS        = m_flash_size - m_flash_res_sect * m_sector_size - 1;
NV_STORAGE_END_ADDRESS    = NV_STORAGE_START_ADDRESS - NV_STORAGE_SIZE + 1;
INT_STORAGE_START         = NV_STORAGE_END_ADDRESS - 1;
INT_STORAGE_END           = (UseInternalStorageLink) ? m_int_start_addr : INT_STORAGE_START;
INT_STORAGE_SIZE          = INT_STORAGE_START - INT_STORAGE_END;

/* OpenThread symbols */
__nv_storage_end_address = NV_STORAGE_START_ADDRESS;
__nv_storage_start_address = NV_STORAGE_END_ADDRESS;

m_text_start              = m_app_start;
m_text_end                = m_app_end;

/* Other Defines */
m_text_size              = m_text_end - m_text_start +1;
INT_STORAGE_SECTOR_SIZE   = m_sector_size;
M_VECTOR_RAM_SIZE         = DEFINED(__ram_vector_table__) ? vector_table_size : 0x0;


/* Specify the memory areas */
MEMORY
{
  /* Define each memory region */
    IntFlash        (RX) : ORIGIN = m_app_start,     LENGTH = m_app_size

    SCRATCH_RAM(rwx)   : ORIGIN = 0x4000000, LENGTH = 0x400   /* 1K bytes (alias SCRATCH_RAM) */
    RAM0 (rwx)    : ORIGIN = 0x4000400, LENGTH = 0x15c00 /* 87K bytes (alias RAM) */
    RAM1 (rwx)    : ORIGIN = 0x4020000, LENGTH = 0x10000 /* 64K bytes (alias RAM2) */
}

  /* Define a symbol for the top of each memory region */
  __base_Flash640 = 0x00000000  ; /* Flash640 */
  __base_Flash = 0x0 ; /* Flash */
  __top_Flash640 = 0x0 + 0xa0000 ; /* 640K bytes */
  __top_Flash = 0x0 + 0xa0000 ; /* 640K bytes */
  __base_unretainedRAM0 = 0x4000000  ; /* RAM0 bank : trampled at Warm boot, can be used as scratch area */
  __top_unretainedRAM0 =  0x4000400  ; /* start of normal use RAM0 */
  __base_RAM0 = 0x4000400  ; /* RAM0 */
  __base_RAM = 0x4000400 ; /* RAM */
  __top_RAM0 = 0x4000400 + 0x15c00 ; /* 87K bytes */
  __top_RAM = 0x4000400 + 0x15c00 ; /* 87K bytes */
  __base_RAM1 = 0x4020000  ; /* RAM1 */
  __base_RAM2 = 0x4020000 ; /* RAM2 */
  __top_RAM1 = 0x4020000 + 0x10000 ; /* 64K bytes */
  __top_RAM2 = 0x4020000 + 0x10000 ; /* 64K bytes */



SECTIONS
{
    /* MAIN TEXT SECTION */
    .m_interrupts : ALIGN(4)
    {
        _flash_start = ABSOLUTE(.);
        _flash_beg = ABSOLUTE(.);

        FILL(0xff)
        __vectors_start__ = m_app_start ;
        __VECTOR_TABLE = .;
        KEEP(*(.isr_vector))
        FILL(0xff)
        . = ALIGN (0x10);
    } > IntFlash

    /* MAIN TEXT SECTION */
    .data_bss : ALIGN(10)
    {
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */
        FILL(0xff)
        . = ALIGN (0x10);
    } > IntFlash

    .ro_nonce : ALIGN(0x10)
    {
        _FlsNonceStart = ABSOLUTE(.);
        *(.ro_nonce) /* nonce value is 16 bytes.*/
        FILL(0xff)
        . = ALIGN (0x10);
    } > IntFlash

    .ro_ota_header : ALIGN(0x10)
    {
        _enc_start = ABSOLUTE(.);
        _enc_offset = (_enc_start & 0x0000000F);
        _FlsOtaHeader = ABSOLUTE(.);
        *(.ro_ota_header) /* Ota Header 69 bytes*/
        FILL(0xff)
        . = ALIGN (0x10);
    } > IntFlash

    .ro_se_lnkKey (ALIGN((. - _enc_offset), 16) + _enc_offset):
    {
        _FlsLinkKey = ABSOLUTE(.);
        *(.ro_se_lnkKey)  /* Link Key 16 bytes*/
        FILL(0xff)
        . = ALIGN (0x10);
    } > IntFlash

    .filler :
    {
        BYTE(0xff)
        FILL(0xff);
        . = ALIGN(0x40);
    } > IntFlash

    .text : ALIGN(0x40)
    {
        FILL(0xff)

       *(.after_vectors*)
       *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
       *crtbegin.o(.ctors)
       *crtbegin?.o(.ctors)
       *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
       *(SORT(.ctors.*))
       *(.ctors)

       /* .dtors */
       *crtbegin.o(.dtors)
       *crtbegin?.o(.dtors)
       *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
       *(SORT(.dtors.*))
       *(.dtors)

        *(.rodata .rodata.* .constdata .constdata.*)
        . = ALIGN(4);
    } > IntFlash

    .NVM_TABLE :
    {
      . = ALIGN(4);
      PROVIDE(__start_NVM_TABLE = .);
      KEEP(*(.NVM_TABLE));
      PROVIDE(__stop_NVM_TABLE = .);
      . = ALIGN(4);
    } > IntFlash

    .VERSION_TAGS :
    {
        . = ALIGN(4);
        PROVIDE(__start_VERSION_TAGS = .);
        KEEP(*(.VERSION_TAGS));
        PROVIDE(__stop_VERSION_TAGS = .);
        . = ALIGN(4);
    } > IntFlash

    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this.
     */
    .ARM.extab : ALIGN(4)
    {
       FILL(0xff)
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > IntFlash
    __exidx_start = .;

    .ARM.exidx : ALIGN(4)
    {
       FILL(0xff)
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > IntFlash
    __exidx_end = .;

    _etext = .;

    /* RAM1/RAM2 (different names for same thing) SECTION */
    /* RAM1 contents are specified before RAM0 as they have specific input
       sections and we do not want the RAM0 wildcards to catch them */
    /* DATA section for RAM1 */
    .data_RAM2 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM2 = .) ;
        *(.ramfunc.$RAM2)
        *(.ramfunc.$RAM1)
        *(.data.$RAM2*)
        *(.data.$RAM1*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM2 = .) ;
     } > RAM1 AT>IntFlash

    .interrupts_ram :
    {
        . = ALIGN(4);
        __VECTOR_RAM__ = .;
        __interrupts_ram_start__ = .;   /* Create a global symbol at data start */
        *(.m_interrupts_ram)            /* This is a user defined section */
        . += M_VECTOR_RAM_SIZE;
        . = ALIGN(4);
        __interrupts_ram_end__ = .;     /* Define a global symbol at data end */
    } > RAM0
    .scratch_area (NOLOAD): ALIGN(4)
    {
       __scratch_area_start__ = .;
       . = ALIGN(4) ;
       . += 0x400;
       __scratch_area_top__ = .;

    } > SCRATCH_RAM

    /* MAIN DATA SECTION */
    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > RAM0

    /* Main DATA section (RAM0) */
    .data : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       *(vtable)
       *(.ramfunc*)
       *(.data*)

       . = ALIGN(4) ;
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        __init_array_start = . ;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = . ;

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))

       . = ALIGN(4) ;

       _edata = . ;
    } > RAM0 AT>IntFlash

    __VECTOR_RAM = __VECTOR_RAM__;
    __RAM_VECTOR_TABLE_SIZE_BYTES = DEFINED(__ram_vector_table__) ? (__interrupts_ram_end__ - __interrupts_ram_start__) : 0x0;

    /* BSS section for RAM1 */
    .bss_RAM2 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       . = ALIGN(4);
       *(.bss_RAM2)
       *(.bss.$RAM2*)
       *(.bss.$RAM1*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
    } > RAM1

    /* MAIN BSS SECTION */
    .bss (NOLOAD) : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        *(g_u32NwkFrameCounter)
        . = ALIGN(4) ;
        _ebss = .;

        PROVIDE(end = .);
    } > RAM0

    /* BSS section for MAC buffers */
    .bss_MAC (NOLOAD) : ALIGN(4)
    {
       /* MAC buffer section: must be within 128kB block. __mac_buffer_base is
          defined further down to be on 128kB alignment */
        __mac_buffer_start = .;
       *(.mac_buffer)

        . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
    } > RAM0

    /* HEAP */
    .heap (NOLOAD): ALIGN(4)
    {
        _heap = .;
        . += HEAP_SIZE;
        . = ALIGN(4) ;
        _end_heap = .;
    } > RAM0

    /* NOINIT section for RAM1 */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
       *(.noinit.$RAM2*)
       *(.noinit.$RAM1*)
       . = ALIGN(4) ;
    } > RAM1

    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        *(.noinit*)
        . = ALIGN(4) ;
        _end_noinit = .;
    } > RAM0

    /* end of firmware RAM to be retained in power down mode */
    _end_fw_retention = .;

    /* non retained RAM section */
    .s_start_non_ret (NOLOAD) : ALIGN(4)
    {
        *(.s_start_non_ret*)
        . = ALIGN(4) ;
    } > RAM0

    /* stack for rom boot during warm resume */
    .boot_resume_stack (NOLOAD): ALIGN(4)
    {
        _boot_resume_stack = .;
        *(.boot_resume_stack*)
        . += BOOT_RESUME_STACK_SIZE;
        . = ALIGN(4) ;
        _end_boot_resume_stack = .;
    } > RAM0

    PROVIDE(_pvHeapStart = _heap);
    PROVIDE(_pvHeapLimit = _pvHeapStart + (HEAP_SIZE));
    PROVIDE(_scratch_buf_start = __scratch_area_start__);
    PROVIDE(_scratch_buf_end = __scratch_area_top__);
    PROVIDE(_vStackTop = DEFINED(__user_stack_top) ? __user_stack_top : __top_RAM0 - 32);
    PROVIDE(__mac_buffer_base = (__mac_buffer_start & 0xfffe0000));

    __StackLimit = _vStackTop - STACK_SIZE;
    ASSERT(__StackLimit >= _end_boot_resume_stack, "Possible stack corruption with data/bss/boot_stack")
}

GROUP(libcr_c.a libcr_eabihelpers.a libcr_newlib_nohost.a)
