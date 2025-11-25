module top_module (
    input [31:0] a,
    input [31:0] b,
    output [31:0] sum
    );
  
  wire cout1, cout2;
  wire [15:0] sum1, sum2;
  
  add16 instance1(.a(a[15:0]), .b(b[15:0]), .cin(0), .cout(cout1), .sum(sum1));
  add16 instance2(.a(a[31:16]), .b(b[31:16]), .cin(cout1), .cout(cout2), .sum(sum2));
    
  assign sum = {sum2, sum1};

endmodule

module add16 (
  input [15:0] a,
  input [15:0] b,
  input cin,
  output [15:0] sum,
  output cout
  );
  
  wire [16:0] carry;
  assign carry[0] = cin;
  
  genvar i;
  generate
    for (i = 0; i < 16; i = i + 1) begin: gen_add
      add1 fa(
        .a(a[i]),
        .b(b[i]),
        .cin(carry[i]),
        .sum(sum[i]),
        .cout(carry[i+1])
        );
    end
  endgenerate
  
  assign cout = carry[16];
  
endmodule

module add1 ( 
  input a,
  input b,
  input cin,
  output sum,
  output cout 
  );
    
	assign sum = a ^ b ^ cin;
  assign cout = a&b | a&cin | b&cin;

endmodule
