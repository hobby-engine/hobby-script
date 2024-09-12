struct Foo {
  static fn bar() {}
}

io:print(Foo:bar()); // expect: null
