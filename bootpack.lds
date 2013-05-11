ENTRY(main)
SECTIONS {
    . = 0x0000c600;
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
