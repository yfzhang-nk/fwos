ENTRY(main)
SECTIONS {
    . = 0x0000a000;
    .head : {
       * (.head); 
    }
    .text : { 
       * (.head); 
       * (.text); 
    }
    .data : {
       * (.data);
    }
}
