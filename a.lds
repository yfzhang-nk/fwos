ENTRY(main)
SECTIONS {
    . = 0x00000010;
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
