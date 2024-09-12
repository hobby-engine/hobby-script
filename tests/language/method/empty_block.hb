struct Foo {
  fn bar() {}
}

io:print(Foo {}.bar()); // expect: null
