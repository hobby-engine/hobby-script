struct Foo {
  var bar;
  var baz;
}

var foo = Foo {};

io:print(foo.bar = "bar value"); // expect: bar value
io:print(foo.baz = "baz value"); // expect: baz value

io:print(foo.bar); // expect: bar value
io:print(foo.baz); // expect: baz value
