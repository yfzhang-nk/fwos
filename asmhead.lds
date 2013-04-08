ENTRY(start)
SECTIONS {
         . = 0x0000c200;
         .text : { 
		 	* (.head); 
		 	* (.text); 
		 }
		 .data : {
		 	* (.data);
		 }
}
