module ir_modulator (
	input clk,
	output out
);

reg [23:0] carrier; // 24-bit counter
reg carrier_out = 0;

always @(posedge clk) begin
	if (carrier < 632) // 38KHz. 8000000 / 2 / 38000
	begin
		carrier <= carrier + 1;
	end else 
	begin
		carrier <= 0;
		carrier_out <= ~carrier_out;
	end
end

assign out = carrier_out;

endmodule