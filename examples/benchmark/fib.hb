fn fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

var s = sys:clock();
for (i = 0; i < 5; i++) {
  io:print(fib(28));
}
io:print("Time: " .. (sys:clock() - s));
