ENTRY(start)
SECTIONS {
         . = 0x0000c800;
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
