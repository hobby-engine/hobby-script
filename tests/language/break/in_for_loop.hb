for(i = 1; ; i++) {
  io:print(i);
  if (i > 2) {
    break;
  }
  io:print(i);
}
// expect: 1
// expect: 1
// expect: 2
// expect: 2
// expect: 3
