from copy import copy
from merge_sort import merge_sort

# check if there are 2 elems in array with their sum == value
def check_sum(arr, value):
    arr_copy = copy(arr)
    merge_sort(arr_copy)
    addition_arr = list(map(lambda x: value - x, arr_copy))
    i = 0
    j = len(arr_copy) - 1
    while i < len(arr_copy) and j >= 0:
        if i == len(arr_copy) - 1 - j:
            if j > 0 and arr_copy[i] == addition_arr[j - 1]:
                return True
            else:
                i += 1
        elif arr_copy[i] == addition_arr[j]:
            return True
        elif arr_copy[i] < addition_arr[j]:
            i += 1
        else:
            j -= 1
    return False
    
arr = list(map(int, input().split()))
value = int(input())
print(check_sum(arr, value))
