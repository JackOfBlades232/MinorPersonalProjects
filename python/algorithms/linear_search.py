def linear_search(arr, value):
    for i in range(len(arr)):
        if arr[i] == value:
            return i
    return -1

arr = list(map(int, input().split()))
value = int(input())
print(linear_search(arr, value))
