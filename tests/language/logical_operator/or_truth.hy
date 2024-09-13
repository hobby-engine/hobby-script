// False and null are false
io:print(false || "ok"); // expect: ok
io:print(null || "ok"); // expect: ok

// Everything else is true
io:print(true || "ok"); // expect: true
io:print(0 || "bad"); // expect: 0
io:print("ok" || "bad"); // expect: ok
