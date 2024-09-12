// Assignment on RHS of variable
var a = "before";
var c = a = "var";
io:print(a); // expect: var
io:print(c); // expect: var
