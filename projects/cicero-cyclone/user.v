/*

To prevent the possibility of chainging the boilerplate, this file gets included
in the top level Verilog file

Notice, though, that Quartus may not figure out you need a compile when you change this
So it will ask you if you want to run the process again and you should say yes.If smart recompile
is on this may not be enough.

*/

module user
(
	input clk,
	input reset_n
);

reg [63:0] data_in;
reg [31:0] address;
reg [31:0] start_cc_pointer;
reg [31:0] end_cc_pointer;
reg [31:0] command;
reg [31:0] status;
reg [63:0] data_out;

Virtual_JTAG_Adapter VJA (
   .           data_in    (data_in),
	.           address    (address),
	.  start_cc_pointer    (start_cc_pointer),
	.    end_cc_pointer    (end_cc_pointer),
	.           command    (command),
	.            status    (status),
	.          data_out    (data_out)
);

wire reset;
assign reset = ~ reset_n;

AXI_top UIP (
   .                        clk			(clk),
   .             	          rst			(reset),
   .           data_in_register			(data_in),
   .           address_register			(address),
   .  start_cc_pointer_register	      (start_cc_pointer),
	.    end_cc_pointer_register        (end_cc_pointer),
   .               cmd_register			(command),
   .            status_register			(status),
   .            data_o_register			(data_out)
);

endmodule