{
  var a = "outer";
  {
    io:print(a); // expect: outer
    var a = "inner";
    io:print(a); // expect: inner
  }
}
