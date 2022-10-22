def selection_sort(arr):
    for j in range(len(arr) - 1):
        min_elem = arr[j]
        min_index = j
        for i in range(j + 1, len(arr)):
            if arr[i] < min_elem:
                min_elem = arr[i]
                min_index = i
        arr[j], arr[min_index] = arr[min_index], arr[j]

arr = list(map(int, input().split()))
selection_sort(arr)
print(arr)
