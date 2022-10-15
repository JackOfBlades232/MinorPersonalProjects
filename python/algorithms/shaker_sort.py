def shaker_sort(arr):
    left = 1
    right = len(arr) - 1
    while right >= left:
        last_exchg_index = left
        for i in range(right, left - 1, -1):
            if arr[i] < arr[i - 1]:
                arr[i], arr[i - 1] = arr[i - 1], arr[i]
                last_exchg_index = i 
        left = last_exchg_index + 1
        last_exchg_index = right
        for i in range(left, right + 1):
            if arr[i] < arr[i - 1]:
                arr[i], arr[i - 1] = arr[i - 1], arr[i]
                last_exchg_index = i 
        right = last_exchg_index - 1

arr = list(map(int, input().split(" ")))
print(arr)
shaker_sort(arr)
print(arr)
