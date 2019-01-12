// simple tests.

testAssert = fun(): assert(true);

testShortCircuit = fun():
    assert(!(false && assert(false)))
    assert(true || assert(false));

testOnlyOneIfBranchEvaled = fun():
    if true: () else assert(false) end
    if false: assert(false) else ();

testSeq = fun(): assert(1 2 3 == 3);

testUnit = fun(): let x: Unit = () in assert(x == ());

testMutual = fun():
    let
        odd: static fun(n: Int) Bool; // forward declare must be static
        even = fun(n: Int) Bool: if n == 0: true else odd(n - 1);
        odd = fun(n): if n == 0: false else even(n - 1)
    in
        assert(even(2))
        assert(!even(5));

// declare type to allow recursion (top level implicitly static)
exp: fun(x, n: Int) Int =
     fun(x, n): if n == 0: 1 else x * exp(x, n - 1);
pow2 = fun(n: Int) Int: exp(2, n);
testExp = fun():
    assert(pow2(3) == 8)
    assert(exp(3, 3) == 27);

testTypeExpr = fun():
    let
        procType = fun(t: Type) Type: t;
        getType = fun(i: Int) Type: if i == 0: Bool else Int;
        test = fun(): let x: procType(getType(1)) = 123 in x
    in
        assert(test() == 123);

fibHelp: fun(a, b, n: Int) Int;
fibHelp = fun(a, b, n): if n == 0: a else fibHelp(b, a+b, n-1);
fib = fun(n: Int) Int: fibHelp(0, 1, n);
testFib = fun():
    assert(fib(6) == 8);

IntFun: Type = fun() Int;
testFunReturn = fun():
    let getAdder = fun(n: Int) IntFun: fun: 99 + n
    in  assert(getAdder(700)() == 799)
        assert(getAdder(1)() == 100);

testFnReturn2 = fun:
    assert((fun(x): fun(y): fun(z): x * y + z)(100)(2)(3) == 203);
        
//vec2 = struct x, y: Int end;

testOps = fun():
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



when = fun(cond, body: Expr) Expr:
    quote(if splice(cond): splice(body) else ());

testWhenRaw = fun(): splice(when(quote(1 == 1), quote(assert(true))));

testWhenWithSugar = fun(): when!(1 == 1,
                            assert(true)
                            when!(1 == 1, assert(true)));

testStatic = fun(): let x = static(pow2(5)) in assert(x == 32);

testStatic2 = fun: let x = static((fun(x): x + 100)(3)) in assert(x == 103);


staticFoo = fun(x: static Int) Int: x * x;
testStaticFoo = fun: assert(staticFoo(2) == 4);

testCallArg = fun:
    let foo = fun(y, x):
        let bar = fun(x, y): x in
        assert(bar(y, x) == 2)
    in
        foo(2, 1);

