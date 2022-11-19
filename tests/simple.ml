// simple tests.

Self.testAssert = fun:
    assert(true)

if true:
    if false:
        assert(false)
else:
    assert(false)

Self.number = 123
assert(Self.number == 123)

const Foo = struct:
    Self.x = 10
    Self.y = 20
    Self.Bar = struct:
        Self.z = 5
        Self.calcSum = fun:
            x + y + z
assert(Foo.x + Foo.y + Foo.Bar.z == 35)
assert(Foo.Bar.calcSum() == 35)

do:
    const str: String = "foo"
    assert(str == "foo")
    assert(str != "bar")

do:
    const Secret = import("tests/secret.ml")
    assert(Secret == import("tests/secret.ml"))
    assert(Secret.getSecret() == 666)

do:
    const f = 0.5

Self.testShortCircuit = fun:
    assert(not (false and assert(false)))
    assert(true or assert(false))

Self.testOnlyOneIfBranchEvaled = fun:
    if true: () else: assert(false)
    if false: assert(false) else: ()

Self.testIfWithManyStatements = fun:
    if false:
        assert(false)
        assert(false)
    elif true:
        assert(true)
        assert(true)
    else:
        assert(false)
        assert(false)

Self.testUnit = fun:
    const x: Unit = ()
    assert(x == ())

Self.testMutual = fun:
    const even = fun(n: Int) -> Bool:
        if n == 0: true else: odd(n - 1)
    const odd = fun(n):
        if n == 0: false else: even(n - 1)
    assert(even(2))
    assert(not even(5))


const exp = fun(x, n):
    if n == 0: 1 else: x * exp(x, n - 1)
const pow2 = fun(n: Int) -> Int: exp(2, n)
Self.testExp = fun:
    assert(pow2(3) == 8)
    assert(exp(3, 3) == 27)

Self.testTypeExpr = fun:
    const procType = fun(t: Type) -> Type: t
    const getType = fun(i: Int) -> Type:
        if i == 0: Bool else: Int
    const test = fun:
        const x: procType(getType(1)) = 123
        x
    assert(test() == 123)

const fibHelp = fun(a, b, n):
    if n == 0: a else: fibHelp(b, a+b, n-1)
const fib = fun(n: Int) -> Int: fibHelp(0, 1, n)
Self.testFib = fun:
    assert(fib(6) == 8)

const IntFun: Type = Fun -> Int
Self.testFunReturn = fun:
    const getAdder = fun(n: Int) -> IntFun:
        fun: 99 + n
    assert(getAdder(700)() == 799)
    assert(getAdder(1)() == 100)

Self.testFnReturn2 = fun:
    assert((fun(x): fun(y): fun(z): x * y + z)(100)(2)(3) == 203)

Self.testOps = fun:
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
    assert(not true == false)
    assert(not false == true)
    assert(~0 == -1)
    assert(false or true)
    assert(true and true)
    assert((1 | 2) == 3)
    assert((3 & 2) == 2)
    assert((123 ^ 123) == 0)
    assert((1 ^ 0) == 1)
    assert(1 << 5 == 32)
    assert(32 >> 4 == 2)



const when = fun(cond: Expr, body: Expr) -> Expr:
    quote(if splice(cond): splice(body) else: ())

Self.testWhenRaw = fun:
    splice(when(quote(1 == 1), quote(assert(true))))

Self.testWhenWithSugar = fun:
    when!(1 == 1, do:
        assert(true)
        when!(1 == 1, assert(true)))

Self.testStatic = fun:
    const x = static(pow2(5))
    assert(x == 32)

Self.testStatic2 = fun:
    const x = static((fun(x): x + 100)(3))
    assert(x == 103)


const staticFoo = fun(x: static Int) -> Int: x * x
Self.testStaticFoo = fun: assert(staticFoo(2) == 4)

Self.testCallArg = fun:
    const foo = fun(y, x):
        const bar = fun(x, y): x
        assert(bar(y, x) == 2)
    foo(2, 1)


// this will require changes to how functions are handled:
Self.testStaticArgAsReturnType = fun:
    const test = fun(t: static Type, x: t) -> t: x
    assert(test(Int, 123) == 123)
