{
  var fib = fn(n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
  };

  io:print(fib(8)); // expect: 21
}
