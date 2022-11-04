def max_subarray_sum(array):
    max_sum = array[0]
    max_sum_ending = array[0]
    for i in range(2, len(array)):
        if max_sum_ending >= 0:
            max_sum_ending += array[i]
        else:
            max_sum_ending = array[i]
        if max_sum < max_sum_ending:
            max_sum = max_sum_ending
    return max_sum

array = list(map(int, input().split()))
print(max_subarray_sum(array))
