// Support module for BCD full-adder used by multiple DUTs
module bcd_fadd(
    input  [3:0] a,
    input  [3:0] b,
    input        cin,
    output       cout,
    output [3:0] sum
);
    wire [4:0] t = a + b + cin;          // 0..19
    assign {cout, sum} = (t > 9) ? {1'b1, t + 5'd6} : {1'b0, t[3:0]};
endmodule

