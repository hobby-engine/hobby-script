struct Foo {
  fn method(a, b) {
    io:print(a);
    io:print(b);
  }
}

Foo {}.method(1, 2, 3, 4); // expect runtime error: Expected 2 arguments, but got 4
