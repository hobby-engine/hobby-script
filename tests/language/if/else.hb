// Evaluate the 'else' expression if the condition is false
if(true) io:print("good"); else io:print("bad"); // expect: good
if(false) io:print("bad"); else io:print("good"); // expect: good

// Allow block body.
if(false) null; else { io:print("block"); } // expect: block
