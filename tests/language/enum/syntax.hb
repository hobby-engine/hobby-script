enum Enum {
  one,
  two,
  three,
}

io:print(Enum:one); // expect: 0
io:print(Enum:two); // expect: 1
io:print(Enum:three); // expect: 2
