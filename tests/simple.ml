// simple tests.

def testAssert = fun() -> assert(true);

def testShortCircuit = fun -> begin
    assert(not (false and assert(false)));
    assert(true or assert(false));
end;

def testOnlyOneIfBranchEvaled = fun -> begin
    if true then () else assert(false) end;
    if false then assert(false) else () end;
end;

def testIfWithManyStatements = fun ->
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

def testUnit = fun -> begin
    def x: Unit = ();
    assert(x == ());
end;

def testMutual = fun -> begin
    def static odd: fun(n: Int): Bool; // forward declare must be static
    def even = fun(n: Int): Bool -> if n == 0 then true else odd(n - 1) end;
    def odd = fun(n) -> if n == 0 then false else even(n - 1) end;
    assert(even(2));
    assert(not even(5));
end;

// declare type to allow recursion (top level implicitly static)
def exp: fun(x, n: Int): Int = fun(x, n) -> if n == 0 then 1 else x * exp(x, n - 1) end;
def pow2 = fun(n: Int): Int -> exp(2, n);
def testExp = fun -> begin
    assert(pow2(3) == 8);
    assert(exp(3, 3) == 27);
end;

def testTypeExpr = fun -> begin
    def procType = fun(t: Type): Type -> t;
    def getType = fun(i: Int): Type ->
        if i == 0 then Bool else Int end;
    def test = fun -> begin
        def x: procType(getType(1)) = 123;
        x
    end;
    assert(test() == 123);
end;

def fibHelp: fun(a, b, n: Int): Int;
def fibHelp = fun(a, b, n) -> if n == 0 then a else fibHelp(b, a+b, n-1) end;
def fib = fun(n: Int): Int -> fibHelp(0, 1, n);
def testFib = fun ->
    assert(fib(6) == 8);

def IntFun: Type = fun(): Int;
def testFunReturn = fun -> begin
    def getAdder = fun(n: Int): IntFun -> fun -> 99 + n;
    assert(getAdder(700)() == 799);
    assert(getAdder(1)() == 100);
end;

def testFnReturn2 = fun ->
    assert((fun(x) -> fun(y) -> fun(z) -> x * y + z)(100)(2)(3) == 203);
        
def Vec2 = struct
    def x: Int = 0;
    def y: Int = 1;
end;

def testOps = fun -> begin
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



def when = fun(cond, body: Expr): Expr ->
    quote(if splice(cond) then splice(body) else () end);

def testWhenRaw = fun -> splice(when(quote(1 == 1), quote(assert(true))));

def testWhenWithSugar = fun -> when!(1 == 1,
                                begin
                                    assert(true);
                                    when!(1 == 1, assert(true))
                                end);

def testStatic = fun -> begin
    def x = static(pow2(5));
    assert(x == 32);
end;

def testStatic2 = fun -> begin
    def x = static((fun(x) -> x + 100)(3));
    assert(x == 103);
end;


def staticFoo = fun(x: static Int): Int -> x * x;
def testStaticFoo = fun -> assert(staticFoo(2) == 4);

def testCallArg = fun -> begin
    def foo = fun(y, x) -> begin
        def bar = fun(x, y) -> x;
        assert(bar(y, x) == 2);
    end;
    foo(2, 1);
end;


// this will require changes to how functions are handled:
//def testStaticArgAsReturnType = fun -> begin
//    def test = fun(t: static Type; x: t): t -> x;
//    assert(test(Int, 123) == 123);
//end;
