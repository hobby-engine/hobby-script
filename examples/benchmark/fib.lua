local function fib(n)
  if n < 2 then return n end
  return fib(n - 2) + fib(n - 1)
end

local start = os.clock()
for _ = 1, 5 do
  print(fib(28))
end
print("Time: " .. os.clock() - start)
