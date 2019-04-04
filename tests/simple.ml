// simple tests.

self.testAssert = fun() -> assert(true);

self.number = 123;
assert(self.number == 123);

const Foo = struct
    self.x = 10;
    self.y = 20;
    self.Bar = struct
        self.z = 5;
        self.calcSum = fun -> x + y + z;
    end;
end;
assert(Foo.x + Foo.y + Foo.Bar.z == 35);
assert(Foo.Bar.calcSum() == 35);

begin
    const str: String = "foo";
    assert(str == "foo");
    assert(str != "bar");
end;

begin
    const Secret = import("tests/secret.ml");
    assert(Secret == import("tests/secret.ml"));
    assert(Secret.getSecret() == 666);
end;

self.testShortCircuit = fun -> begin
    assert(not (false and assert(false)));
    assert(true or assert(false));
end;

self.testOnlyOneIfBranchEvaled = fun -> begin
    if true then () else assert(false) end;
    if false then assert(false) else () end;
end;

self.testIfWithManyStatements = fun ->
    if false then
        assert(false);
        assert(false)
    elif true then
        assert(true);
        assert(true);
    else
        assert(false);
        assert(false);
    end;

self.testUnit = fun -> begin
    const x: Unit = ();
    assert(x == ());
end;

self.testMutual = fun -> begin
    const static odd: fun(n: Int): Bool; // forward declare must be static
    const even = fun(n: Int): Bool -> if n == 0 then true else odd(n - 1) end;
    const odd = fun(n) -> if n == 0 then false else even(n - 1) end;
    assert(even(2));
    assert(not even(5));
end;

// declare type to allow recursion (top level implicitly static)
const exp: fun(x, n: Int): Int = fun(x, n) -> if n == 0 then 1 else x * exp(x, n - 1) end;
const pow2 = fun(n: Int): Int -> exp(2, n);
self.testExp = fun -> begin
    assert(pow2(3) == 8);
    assert(exp(3, 3) == 27);
end;

self.testTypeExpr = fun -> begin
    const procType = fun(t: Type): Type -> t;
    const getType = fun(i: Int): Type ->
        if i == 0 then Bool else Int end;
    const test = fun -> begin
        const x: procType(getType(1)) = 123;
        x
    end;
    assert(test() == 123);
end;

const fibHelp: fun(a, b, n: Int): Int;
const fibHelp = fun(a, b, n) -> if n == 0 then a else fibHelp(b, a+b, n-1) end;
const fib = fun(n: Int): Int -> fibHelp(0, 1, n);
self.testFib = fun ->
    assert(fib(6) == 8);

const IntFun: Type = fun(): Int;
self.testFunReturn = fun -> begin
    const getAdder = fun(n: Int): IntFun -> fun -> 99 + n;
    assert(getAdder(700)() == 799);
    assert(getAdder(1)() == 100);
end;

self.testFnReturn2 = fun ->
    assert((fun(x) -> fun(y) -> fun(z) -> x * y + z)(100)(2)(3) == 203);

self.testOps = fun -> begin
    assert(1 == 1);
    assert(1 != 2);
    assert(1 < 2);
    assert(2 > 1);
    assert(2 <= 2);
    assert(1 <= 2);
    assert(2 >= 2);
    assert(2 >= 1);
    assert(1 + 2 == 3);
    assert(3 - 2 == 1);
    assert(2 - 3 == -1);
    assert(+1 == 1);
    assert(3 * 2 == 6);
    assert(12 / 3 == 4);
    assert(15 % 10 == 5);
    assert(not true == false);
    assert(not false == true);
    assert(~0 == -1);
    assert(false or true);
    assert(true and true);
    assert((1 | 2) == 3);
    assert((3 & 2) == 2);
    assert((123 ^ 123) == 0);
    assert((1 ^ 0) == 1);
    assert(1 << 5 == 32);
    assert(32 >> 4 == 2);
end;



const when = fun(cond, body: Expr): Expr ->
    quote(if splice(cond) then splice(body) else () end);

self.testWhenRaw = fun -> splice(when(quote(1 == 1), quote(assert(true))));

self.testWhenWithSugar = fun -> when!(1 == 1,
                                begin
                                    assert(true);
                                    when!(1 == 1, assert(true))
                                end);

self.testStatic = fun -> begin
    const x = static(pow2(5));
    assert(x == 32);
end;

self.testStatic2 = fun -> begin
    const x = static((fun(x) -> x + 100)(3));
    assert(x == 103);
end;


const staticFoo = fun(x: static Int): Int -> x * x;
self.testStaticFoo = fun -> assert(staticFoo(2) == 4);

self.testCallArg = fun -> begin
    const foo = fun(y, x) -> begin
        const bar = fun(x, y) -> x;
        assert(bar(y, x) == 2);
    end;
    foo(2, 1);
end;


// this will require changes to how functions are handled:
//self.testStaticArgAsReturnType = fun -> begin
//    const test = fun(t: static Type; x: t): t -> x;
//    assert(test(Int, 123) == 123);
//end;
