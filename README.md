# Hobby Script
A dynamically typed programming language designed for use in game development.
Hobby Script is designed for use in the future Hobby Game Engine, although it
is standalone as well. 
### Features
- No OOP
- Blazingly average speed!!

## Building
It should be as easy as cloning the repo and going:
```shell
make
```
Requires MinGW on Windows.
## Snippets
Hello world:
```zig
io:print("Hello, world!");
```

Here's timer, so you can get a better feel for the language:
```zig
// Timer.hby
struct Timer;

var total_time = 0;
var time_left = 0;

static fn new(total_time) -> Timer{total_time=time};

fn is_over() -> self.time_left <= 0;
fn step(dt) {
  self.time_left -= dt;
}
```
Then, you can use it in some other file like this:
```zig
// main.hby
var Timer = import("Timer.hby");

var timer = Timer:new(5);
timer.step(1);
```

