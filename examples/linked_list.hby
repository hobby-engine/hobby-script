struct Node {
  var val;
  var prev;
  var next;

  fn tostr() => "<Node " .. self.val .. ">";
}

struct LinkedList {
  var head;
  var tail;

  static fn new() => LinkedList{};

  fn prepend(val) {
    var node = Node{
      val = val,
      next = self.head,
    };
    if (node.next) {
      node.next.prev = node;
    }
    self.head = node;
    if (!self.tail) {
      self.tail = node;
    }
  }

  fn append(val) {
    var node = Node{
      val = val,
      prev = self.tail,
    };
    if (node.prev) {
      node.prev.next = node;
    }
    self.tail = node;
    if (!self.head) {
      self.head = node;
    }
  }
}

var list = LinkedList:new();
for (i = 0; i < 10; i++) {
  list.prepend(i);
  list.append(i);
}

var node = list.head;
while (node) {
  io:print(node);
  node = node.next;
}
