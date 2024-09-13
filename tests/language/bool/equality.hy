io:print(true == true);    // expect: true
io:print(true == false);   // expect: false
io:print(false == true);   // expect: false
io:print(false == false);  // expect: true

// Not equal to other types
io:print(true == 1);        // expect: false
io:print(false == 0);       // expect: false
io:print(true == "true");   // expect: false
io:print(false == "false"); // expect: false
io:print(false == "");      // expect: false

io:print(true != true);    // expect: false
io:print(true != false);   // expect: true
io:print(false != true);   // expect: true
io:print(false != false);  // expect: false

// Not equal to other types
io:print(true != 1);        // expect: true
io:print(false != 0);       // expect: true
io:print(true != "true");   // expect: true
io:print(false != "false"); // expect: true
io:print(false != "");      // expect: true
