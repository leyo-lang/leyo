# HARD FUNCTION STRESS TEST (Python version)

def a():
    return 1

def b():
    return a() + 2

def c():
    return b() + a()

def d():
    return c() + b() + a()

def e():
    return d() + c() + b()

def f():
    return e() + d()

def g():
    return f() + e() + d() + c()

# deep nesting + repeated calls

def h():
    return g() + g() + f() + e()

def i():
    return h() + g() + f() + e() + d()

# unreachable code + return validation

def j():
    return i()
    # unreachable line (never runs)
    return 999

# entry function

def main():
    return j() + i() + h()


print(main())