var i = -1;
while ((i += 1) < 10): whileLoop {
  if (i % 2 != 0)
    continue whileLoop;

  io:print(i);
}

// expect: 0
// expect: 2
// expect: 4
// expect: 6
// expect: 8
