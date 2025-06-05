SECTIONS {
 .rodata.embox.dtb : {
 _dtb_start = .;
 *(.rodata.embox.dtb)      
 _dtb_end = .;
 }
}