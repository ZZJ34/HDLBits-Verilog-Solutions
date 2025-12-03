module top_module (
    input clk,
    input reset,
    output OneHertz,
    output [2:0] c_enable
  );
   
	wire [3:0] q0, q1, q2;

	bcdcount counter0 (clk, reset, c_enable[0], q0);
	bcdcount counter1 (clk, reset, c_enable[1], q1);
	bcdcount counter2 (clk, reset, c_enable[2], q2);

	assign c_enable = {(q1 == 4'd9) & (q0 == 4'd9), q0 == 4'd9, 1'b1};
	assign OneHertz = (q2 == 4'd9) & (q1 == 4'd9) & (q0 == 4'd9);
    
endmodule

// Simple BCD decade counter 0..9 with enable.
module bcdcount(
    input clk,
    input reset,
    input enable,
    output reg [3:0] q
);
    always @(posedge clk) begin
        if (reset) q <= 4'd0;
        else if (enable) begin
            if (q == 4'd9) q <= 4'd0;
            else q <= q + 4'd1;
        end
    end
endmodule
