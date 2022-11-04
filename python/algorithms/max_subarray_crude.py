def max_subarray_sum(array):
    max_sum = array[0]
    for i in range(0, len(array)):
        cur_sum = 0
        for j in range(i, len(array)):
            cur_sum += array[j]
            if cur_sum > max_sum:
                max_sum = cur_sum
    return max_sum

array = list(map(int, input().split()))
print(max_subarray_sum(array))
