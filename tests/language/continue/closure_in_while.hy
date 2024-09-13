var f;
while(f == null) {
  var i = "i";
  f = fn() => io:print(i);
  continue;
}

f();
// expect: i
