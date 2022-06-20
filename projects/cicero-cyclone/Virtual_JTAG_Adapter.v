module Virtual_JTAG_Adapter
(
  input  reg [31:0]    status,
  output reg [31:0]    command,
  output reg [31:0]    address,
  output reg [31:0]    start_cc_pointer,
  output reg [31:0]    end_cc_pointer,
  output reg [63:0]    data_in,
  input  reg [63:0]    data_out
);

  // Virtual JTAG wires
  wire tck;
  wire tdi;
  wire tdo;
  wire [3:0] ir_in;
  // Virtual JTAG TAP controller states
  wire vJTAG_cdr;
  wire vJTAG_sdr;
  wire vJTAG_udr;
  wire vJTAG_e1dr;
  wire vJTAG_pdr;

  // Virtual JTAG instance
  sld_virtual_jtag #(
  	.sld_auto_instance_index ("YES"),
  	.sld_instance_index      (0),
  	.sld_ir_width            (4)
  ) virtual_jtag_0 (
  	.tdi                (tdi),
  	.tdo                (tdo),
  	.ir_in              (ir_in),
  	.ir_out             (),
  	.virtual_state_cdr  (vJTAG_cdr),
  	.virtual_state_sdr  (vJTAG_sdr),
  	.virtual_state_e1dr (vJTAG_e1dr),
  	.virtual_state_pdr  (vJTAG_pdr),
  	.virtual_state_e2dr (),
  	.virtual_state_udr  (vJTAG_udr),
  	.virtual_state_cir  (),
  	.virtual_state_uir  (),
  	.tck                (tck)
  );
  
  // Virtual JTAG IR possible values
  localparam VIR_BYPASS    = 4'b0000;
  localparam VIR_STATUS    = 4'b0001;
  localparam VIR_COMMAND   = 4'b0010;
  localparam VIR_ADDRESS   = 4'b0011;
  localparam VIR_START_CC  = 4'b0100;
  localparam VIR_END_CC    = 4'b0101;
  localparam VIR_DATA_IN   = 4'b0110;
  localparam VIR_DATA_OUT  = 4'b0111;
  localparam VIR_DEBUG     = 4'b1111;
  
  // Registers where data will be shifted in
  reg           shift_bypass;
  reg [31:0]    shift_status;
  reg [31:0]    shift_command;
  reg [31:0]    shift_address;
  reg [31:0]    shift_start_cc_pointer;
  reg [31:0]    shift_end_cc_pointer;
  reg [63:0]    shift_data_in;
  reg [63:0]    shift_data_out;
  reg [31:0]    shift_debug;
  
  // Virtual JTAG chain
  always @(posedge tck)
   begin
	   case(ir_in)
		   VIR_STATUS:    begin
			                  if (vJTAG_cdr == 1'b1) shift_status = status;
							      if (vJTAG_sdr == 1'b1) shift_status = {tdi, shift_status[31:1]};
			               end
			VIR_COMMAND:   begin
			                  if (vJTAG_sdr == 1'b1) shift_command = {tdi, shift_command[31:1]};
							      if (vJTAG_udr == 1'b1) command = shift_command;
			               end
			VIR_ADDRESS:   begin
			                  if (vJTAG_sdr == 1'b1) shift_address = {tdi, shift_address[31:1]};
							      if (vJTAG_udr == 1'b1) address = shift_address;
			               end
			VIR_START_CC:  begin
			                  if (vJTAG_sdr == 1'b1) shift_start_cc_pointer = {tdi, shift_start_cc_pointer[31:1]};
							      if (vJTAG_udr == 1'b1) start_cc_pointer = shift_start_cc_pointer;
			               end
			VIR_END_CC:    begin
			                  if (vJTAG_sdr == 1'b1) shift_end_cc_pointer = {tdi, shift_end_cc_pointer[31:1]};
							      if (vJTAG_udr == 1'b1) end_cc_pointer = shift_end_cc_pointer;
			               end
			VIR_DATA_IN:   begin
			                  if (vJTAG_sdr == 1'b1) shift_data_in = {tdi, shift_data_in[63:1]};
							      if (vJTAG_udr == 1'b1) data_in = shift_data_in;
			               end
	      VIR_DATA_OUT:  begin
			                  if (vJTAG_cdr == 1'b1) shift_data_out = data_out;
							      if (vJTAG_sdr == 1'b1) shift_data_out = {tdi, shift_data_out[63:1]};
			               end
			VIR_DEBUG:     begin
			                  if (vJTAG_cdr == 1'b1) shift_debug = 32'h123456;
							      if (vJTAG_sdr == 1'b1) shift_debug = {tdi, shift_debug[31:1]};
			               end
			default:       begin
							      if (vJTAG_sdr == 1'b1) shift_bypass = tdi;
			               end
	   endcase
	end
	
  // Always maintain TDI to TDO connectivity
  assign tdo =  (ir_in == VIR_STATUS)   ? shift_status[0]
              : (ir_in == VIR_COMMAND)  ? shift_command[0]
              : (ir_in == VIR_ADDRESS)  ? shift_address[0]
              : (ir_in == VIR_START_CC) ? shift_start_cc_pointer[0]
              : (ir_in == VIR_END_CC)   ? shift_end_cc_pointer[0]
              : (ir_in == VIR_DATA_IN)  ? shift_data_in[0]
              : (ir_in == VIR_DATA_OUT) ? shift_data_out[0]
              : (ir_in == VIR_DEBUG)    ? shift_debug[0]
 	           : shift_bypass;

endmodule