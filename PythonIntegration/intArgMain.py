import testSupport

def multiply(a,b):
    print "Enter runFileMain multiply"
    c = 0
    for i in range(0, a):
        c = c + b
    print "Expected Result: ", a*b
    print "Actual Result: ",c
    testSupport.main(a,b)
    return c

def mul(a,b):
    print "Enter runFileMain mul"
    c = multiply(a,b)
    return c
def main(a,b):
    print "Enter runFileMain main"
    c = mul(a,b)
    return c

