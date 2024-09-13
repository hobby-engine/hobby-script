var f;

struct Foo {
  fn method(param) {
    f = fn() {
      io:print(param);
    };
  }
}

Foo {}.method("param");
f(); // expect: param
