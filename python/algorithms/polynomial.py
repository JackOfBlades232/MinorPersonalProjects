def calculate_polynomial_simple(coeffs, x):    
    y = 0
    for i in range(len(coeffs)):
        elem = coeffs[i]
        for j in range(i):
            elem *= x
        y += elem
    return y

def gorner_rule(coeffs, x):
    y = 0
    for i in range(len(coeffs) - 1, -1, -1):
        y = x * y + coeffs[i]
    return y

coeffs = list(map(int, input().split()))
x = int(input())

y_simple = calculate_polynomial_simple(coeffs, x)
y_gorner = gorner_rule(coeffs, x)

if y_simple == y_gorner:
    print(y_simple)
else:
    print("There's a bug)")
