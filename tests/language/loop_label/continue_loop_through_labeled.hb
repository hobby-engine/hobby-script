var i = 0;
while (i < 3): outer {
  while (true): middle {
    io:print("middle");
    while (true): inner {
      i += 1;
      io:print("inner");
      continue outer;
    }
  }
  io:print("outer");
}

// expect: middle
// expect: inner
// expect: middle
// expect: inner
// expect: middle
// expect: inner

io:print("ok"); // expect: ok
