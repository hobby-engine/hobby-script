var f;

fn foo(param) {
  f = fn() {
    io:print(param);
  };
}
foo("param");

f(); // expect: param
