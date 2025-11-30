module top_module( 
    input  [2:0] a, b,
    input        cin,
    output reg [2:0] cout,
    output reg [2:0] sum 
    );
  
  integer i;
  always @(*) begin
      // Ripple-carry chain computed procedurally to avoid assigning to wires
      cout[0] = (a[0] & b[0]) | (a[0] & cin) | (b[0] & cin);
      sum[0]  = a[0] ^ b[0] ^ cin;
      for (i=1; i<3; i=i+1) begin
          sum[i]  = a[i] ^ b[i] ^ cout[i-1];
          cout[i] = (a[i] & b[i]) | (a[i] & cout[i-1]) | (b[i] & cout[i-1]);
      end
  end

endmodule
