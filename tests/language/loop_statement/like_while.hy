var x = 0;
loop {
  io:print(x);
  if (x == 5) {
    break;
  }
  x += 1;
}

// expect: 0
// expect: 1
// expect: 2
// expect: 3
// expect: 4
// expect: 5
