var a = [];
var b = [];

var a_push = a.push;
var b_push = b.push;

a_push(1);
io:print(a); // expect: [1]

b_push(2);
io:print(b); // expect: [2]
