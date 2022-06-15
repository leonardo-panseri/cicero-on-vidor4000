/*

To prevent the possibility of chainging the boilerplate, this file gets included
in the top level Verilog file

Notice, though, that Quartus may not figure out you need a compile when you change this
So it will ask you if you want to run the process again and you should say yes.If smart recompile
is on this may not be enough.

*/

// Your code goes here:

reg [31:0] data_in = 32'h23;
reg [31:0] address;
reg [31:0] start_cc_pointer;
reg [31:0] end_cc_pointer;
reg [31:0] command;
reg [31:0] status;
reg [31:0] data_out;

Virtual_JTAG_Adapter VJA (
   .           data_in    (data_in),
	.           address    (address),
	.  start_cc_pointer    (start_cc_pointer),
	.    end_cc_pointer    (end_cc_pointer),
	.           command    (command),
	.            status    (status),
	.          data_out    (data_out)
);



assign bMKR_D[6] = data_in == 32'h23;

//always @(posedge wOSC_CLK)
//begin
//  if (!rRESETCNT[5])
//  begin
//     hadcounter<=28'hfffffff;
//  end
//  else
//  begin
//	 if (hadcounter==28'h0) hadcounter<=28'hffffffff; else hadcounter<=hadcounter-28'h1;
//  end
//end


//AXI_top UIP (
//   .                        clk			(wOSC_CLK),
//   .             	          rst			(iRESETn),
//   .           data_in_register			(data_in),
//   .           address_register			(address),
//   .  start_cc_pointer_register	      (start_cc_pointer),
//	.    end_cc_pointer_register        (end_cc_pointer),
//   .               cmd_register			(command),
//   .            status_register			(status),
//   .            data_o_register			(data_out)
//);