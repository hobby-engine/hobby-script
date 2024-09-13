io:print(false && "bad"); // expect: false
io:print(null && "bad"); // expect: null
io:print(true && "ok"); // expect: ok
io:print(0 && "ok"); // expect: ok
io:print("" && "ok"); // expect: ok
