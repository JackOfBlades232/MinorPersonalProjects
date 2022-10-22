def binary_search(arr, value):
    l = 0
    r = len(arr)
    while l < r - 1:
        mid = int((l + r) / 2)
        if arr[mid] <= value:
            l = mid
        else:
            r = mid
    if arr[l] == value:
        return l
    else:
        return -1

arr = list(map(int, input().split()))
value = int(input())
print(binary_search(arr, value))
