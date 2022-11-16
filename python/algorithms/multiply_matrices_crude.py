from copy import deepcopy

def multiply_square_matrices(a, b):
    c = deepcopy(a)
    for i in range(len(a)):
        for j in range(len(a)):
            c[i][j] = 0
            for k in range(len(a)):
                c[i][j] += a[i][k] * b[k][j]
    return c

dim = int(input())
print()
a = []
b = []
for i in range(dim):
    a += [list(map(int, input().split()))]
print()
for i in range(dim):
    b += [list(map(int, input().split()))]
print()
print(multiply_square_matrices(a, b))
