io:print("(" .. "" .. ")");   // expect: ()
io:print("a string"); // expect: a string

// Non-ASCII.
io:print("A~¶Þॐஃ"); // expect: A~¶Þॐஃ
