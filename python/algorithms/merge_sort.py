import copy

def merge_sort(arr):
    merge_sort_step(arr, 0, len(arr))

def merge_sort_step(arr, l, r):
    if l < r - 1:
        mid = int((l + r) / 2)
        merge_sort_step(arr, l, mid)
        merge_sort_step(arr, mid, r)
        merge(arr, l, r, mid)

def merge(arr, l, r, sep):
    if l < sep < r:
        arr1 = copy.copy(arr[l:sep])
        arr2 = copy.copy(arr[sep:r])
        i = 0
        j = 0
        while i < sep - l and j < r - sep:
            if arr1[i] <= arr2[j]:
                arr[l + i + j] = arr1[i]
                i += 1
            else:
                arr[l + i + j] = arr2[j]
                j += 1
        # if there are elems left in arr2, they are already in place
        while i < sep - l:
            arr[l + i + j] = arr1[i]
            i += 1
        del arr1
        del arr2

# arr = list(map(int, input().split()))
# merge_sort(arr)
# print(arr)
