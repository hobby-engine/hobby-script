var a = "a";
var b = "b";
var c = "c";

// Assignment is right-associative
a = b = c;
io:print(a);    // expect: c
io:print(b);    // expect: c
io:print(c);    // expect: c
