var arr = [1, 2, 3, 4];
arr[0] = 0;
io:print(arr); // expect: [0, 2, 3, 4]

// The other shorthand operators use the same code, so if this fails, they all
// fail.
arr[1] += 5;
io:print(arr); // expect: [0, 7, 3, 4]

// Ditto
arr[2]++;
io:print(arr); // expect: [0, 7, 4, 4]
