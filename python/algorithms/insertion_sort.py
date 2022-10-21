def insertion_sort_desc(arr):
    for j in range(len(arr) - 2, -1, -1):
        elem = arr[j]
        i = j + 1
        while i < len(arr) and arr[i] > elem:
            arr[i - 1] = arr[i]
            i += 1
        arr[i - 1] = elem

arr = list(map(int, input().split()))
print(arr)
insertion_sort_desc(arr)
print(arr)
