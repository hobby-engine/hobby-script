// Single statement
for (i = 1; i <= 3; i++)
  io:print(i);
// expect: 1
// expect: 2
// expect: 3


// Block
for (i = 0; i < 3; i++) {
  io:print(i);
}
// expect: 0
// expect: 1
// expect: 2

// Exclude var init
var e = 0;
for (; e < 3; e++)
  io:print(e);
// expect: 0
// expect: 1
// expect: 2

// Exclude condition
for (i = 0; ; i++) {
  if (i >= 3) {
    break;
  }
  io:print(i);
}
// expect: 0
// expect: 1
// expect: 2

// Exclude increment
for (i = 0; i < 3;) {
  io:print(i);
  i++;
}
// expect: 0
// expect: 1
// expect: 2
