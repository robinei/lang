// simple tests.

testAssert = fn(): assert(true);

testShortCircuit = fn():
    assert(!(false && assert(false)))
    assert(true || assert(false));

testOnlyOneIfBranchEvaled = fn():
    if true: () else assert(false) end
    if false: assert(false) else ();

testSeq = fn(): assert(1 2 3 == 3);

testUnit = fn(): let x: Unit = () in assert(x == ());

testMutual = fn():
    let
        even = fn(n: Int) Bool: if n == 0: true else odd(n - 1);
        odd = fn(n: Int) Bool: if n == 0: false else even(n - 1)
    in
        assert(even(2))
        assert(!even(5));

exp = fn(x, n: Int) Int: if n == 0: 1 else x * exp(x, n - 1);
pow2 = fn(n: Int) Int: exp(2, n);
testExp = fn():
    assert(pow2(3) == 8)
    assert(exp(3, 3) == 27);

testTypeExpr = fn():
    let
        getType = fn(i: Int) Type: if i == 0: Bool else Int;
        test = fn(): let x: getType(1) = 123 in x
    in
        assert(test() == 123);

fibHelp = fn(a, b, n: Int) Int: if n == 0: a else fibHelp(b, a+b, n-1);
fib = fn(n: Int) Int: fibHelp(0, 1, n);
testFib = fn():
    assert(fib(6) == 8);

testLetOrder = fn():
    let
        x = add3(2);
        add3 = fn(n): n + 3
    in
        assert(x == 5);

vec2 = struct x, y: Int end;

testOps = fn():
    assert(1 == 1)
    assert(1 != 2)
    assert(1 < 2)
    assert(2 > 1)
    assert(2 <= 2)
    assert(1 <= 2)
    assert(2 >= 2)
    assert(2 >= 1)
    assert(1 + 2 == 3)
    assert(3 - 2 == 1)
    assert(2 - 3 == -1)
    assert(+1 == 1)
    assert(3 * 2 == 6)
    assert(12 / 3 == 4)
    assert(15 % 10 == 5)
    assert(!true == false)
    assert(!false == true)
    assert(~0 == -1)
    assert(false || true)
    assert(true && true)
    assert((1 | 2) == 3)
    assert((3 & 2) == 2)
    assert((123 ^ 123) == 0)
    assert((1 ^ 0) == 1)
    assert(1 << 5 == 32)
    assert(32 >> 4 == 2)
    ;



when = fn(cond, body: Expr) Expr:
    quote(if splice(cond): splice(body) else ());

testWhenRaw = fn(): splice(when(quote(1 == 1), quote(assert(true))));

testWhenWithSugar = fn(): when!(1 == 1, assert(true));
