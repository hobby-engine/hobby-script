var i = 0;
for (;; i++) {
  if (i == 10) {
    break;
  }
}

io:print(i); // expect: 10
