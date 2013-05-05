ENTRY(main)
SECTIONS {
    . = 0x0000cc00;
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
