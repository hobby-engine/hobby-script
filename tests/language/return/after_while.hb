fn f() {
  while(true) return "ok";
}

io:print(f()); // expect: ok
