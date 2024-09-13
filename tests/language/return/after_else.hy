fn f() {
  if(false) "no"; else return "ok";
}

io:print(f()); // expect: ok
