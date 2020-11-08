`include "ws2812.v"
`include "../sysmgr.v"

module top(input clk, input [7:0] btn, output [5:0] ledc, output neopixel); 
	// ledstuff
	localparam NUM_LEDS = 64;
	reg reset = 1;

	wire clk24m;
	wire clk48m;
	wire clk96m;
	wire rst_soc;
	sysmgr sysmgr_I (
		.clk_in(clk),
		.rst_in(1'b0),
		.clk_24m(clk24m),
		.clk_48m(clk48m),
		.clk_96m(clk96m),
		.rst_out(rst_soc)
	);

	always @(posedge clk)
		reset <= 0;

	reg [18:0] count = 0;
	reg [1:0]  color_ind = 0;

	always @(posedge clk) begin
		count <= count + 1;
		if (&count) begin
			if (led_num == NUM_LEDS) begin
				led_num <= 0;
				color_ind <= color_ind + 1;
				case (color_ind)
					2'b00 : led_rgb_data <= 24'h10_00_00;
					2'b01 : led_rgb_data <= 24'h00_10_00;
					2'b10 : led_rgb_data <= 24'h00_00_10;
					2'b11 : led_rgb_data <= 24'h10_10_10;
				endcase
			end else
				led_num <= led_num + 1;
		end
	end

	reg [23:0] led_rgb_data = 24'h00_00_10;
	reg [7:0] led_num = 0;
	wire led_write = &count;

	ws2812 #(.NUM_LEDS(NUM_LEDS)) ws2812_inst(.data(neopixel), .clk(clk96m), .reset(reset), .rgb_data(led_rgb_data), .led_num(led_num), .write(led_write));

	// To make sure it's working...
	assign ledc[0] = ~btn[4]; // 6 LEDs on the top. led[0],led[1],led[2],led[3],led[4],led[5]

endmodule
