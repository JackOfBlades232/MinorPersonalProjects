def add_binary(num1, num2):
    arg_error = ValueError("Numbers must be arrays of 0 and 1 of same length!")
    if len(num1) != len(num2) or len(num1) == 0:
        raise arg_error
    res = [0] * (len(num1) + 1)
    residual = 0
    for i in range(len(num1)):
        if not (0 <= num1[i] <= 1) or not (0 <= num2[i] <= 1):
            raise arg_error
        res[i] = num1[i] + num2[i] + residual
        if res[i] >= 2:
            res[i] -= 2
            residual = 1
    res[-1] = residual
    return res

num1 = list(map(int, list(input())))
num2 = list(map(int, list(input())))
print(add_binary(num1, num2))
