def multiply(a,b):
    print "Enter runFileMain multiply"
    c = 0
    for i in range(0, a):
        c = c + b
    print "Expected Result: 6"
    print "Actual Result: ",c
    return c

def mul(a,b):
    print "Enter runFileMain mul"
    c = multiply(a,b)
    return c
def main():
    print "Enter runFileMain main"
    mul(2,3)
    return

main()
