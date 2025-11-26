module top_module ( 
    input a, 
    input b, 
    input c,
    input d,
    output out1,
    output out2
    );
  
  mod_a inst(
    out1,
    out2,
    a,
    b,
    c,
    d
    );
  
endmodule

module mod_a (
  output out1,
  output out2,
  input in1,
  input in2,
  input in3,
  input in4
  );
  
  assign out1 = (in1 & in2) | (in3 & in4);
  assign out2 = (in1 | in2) & (in3 | in4);
  
endmodule
