// Note: These tests implicitly depend on ints being truthy

// Return the first non-true argument
io:print(false && 1); // expect: false
io:print(true && 1); // expect: 1
io:print(1 && 2 && false); // expect: false

// Return the last argument if all are true
io:print(1 && true); // expect: true
io:print(1 && 2 && 3); // expect: 3

// Short-circuit at the first false argument
var a = "before";
var b = "before";
(a = true) && (b = false) && (a = "bad");
io:print(a); // expect: true
io:print(b); // expect: false
