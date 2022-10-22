import copy
import re

def merge_count_inv(arr):
    arr_copy = copy.copy(arr)
    return merge_sort_and_count_inv_step(arr_copy, 0, len(arr_copy))

def merge_sort_and_count_inv_step(arr, l, r):
    if l < r - 1:
        mid = int((l + r) / 2)
        inv1 = merge_sort_and_count_inv_step(arr, l, mid)
        inv2 = merge_sort_and_count_inv_step(arr, mid, r)
        inv_mutual = merge_and_count_inv(arr, l, r, mid)
        return inv1 + inv2 + inv_mutual
    else:
        return 0;

def merge_and_count_inv(arr, l, r, sep):
    if l < sep < r:
        arr1 = copy.copy(arr[l:sep])
        arr2 = copy.copy(arr[sep:r])
        i = 0
        j = 0
        inv = 0
        while i < sep - l and j < r - sep:
            if arr1[i] <= arr2[j]:
                arr[l + i + j] = arr1[i]
                i += 1
            else:
                arr[l + i + j] = arr2[j]
                inv += len(arr1) - i
                j += 1
        # if there are elems left in arr2, they are already in place
        while i < sep - l:
            arr[l + i + j] = arr1[i]
            i += 1
        del arr1
        del arr2
        return inv
    else:
        return 0;

arr = list(map(int, input().split()))
print(merge_count_inv(arr))
