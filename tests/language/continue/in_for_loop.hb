for (i = 0; ; i++) {
  io:print(i);
  if (i <= 2)
    continue;
  io:print(i);
  break;
}
// expect: 0
// expect: 1
// expect: 2
// expect: 3
// expect: 3
