var f;

fn f1() {
  var a = "a";
  var f2 = fn() {
    var b = "b";
    var f3 = fn() {
      var c = "c";
      f = fn() {
        io:print(a);
        io:print(b);
        io:print(c);
      };
    };
    f3();
  };
  f2();
}
f1();

f();
// expect: a
// expect: b
// expect: c
