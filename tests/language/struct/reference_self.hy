struct Foo {
  fn this() {
    return Foo;
  }
}

io:print(Foo {}.this()); // expect: <Foo>
