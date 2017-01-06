def multiply(a,b):
    print "Enter support multiply"
    c = 0
    for i in range(0, a):
        c = c + b
    print "Expected Support Result: ", a*b
    print "Actual Support Result: ",c
    return c

def mul(a,b):
    print "Enter support mul"
    c = multiply(a,b)
    return c
def main(a,b):
    print "Enter support main"
    mul(a,b)
    return

