struct Tree {
  var item;
  var lhs;
  var rhs;

  static fn new(item, depth) {
    var lhs;
    var rhs;

    if (depth > 0) {
      var item2 = item + item;
      depth -= 1;
      lhs = Tree:new(item2 - 1, depth);
      rhs = Tree:new(item2, depth);
    }

    return Tree {
      item = item,
      lhs = rhs,
      rhs = lhs,
    };
  }

  fn check() {
    if (!self.lhs) {
      return self.item;
    }

    return self.item + self.lhs.check() - self.rhs.check(); 
  }
}

var min_depth = 4;
var max_depth = 12;
var stretch_depth = max_depth + 1;

var start = sys:clock();

var long_lived_tree = Tree:new(0, max_depth);

var iters = 1;
for (i = 0; i < max_depth; i++) {
  iters *= 2;
}

var depth = min_depth;
while (depth < stretch_depth) {
  var check = 0;
  for (i = 1; i < iters; i++) {
    check += Tree:new(i, depth).check() + Tree:new(-i, depth).check();
  }

  io:print((iters * 2) .. " trees of depth " .. depth .. " check: " .. check);
  iterations /= 4;
  depth += 2;
}

io:print("Time: " .. sys:clock() - start);
