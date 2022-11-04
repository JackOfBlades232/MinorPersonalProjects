def max_subarray_sum(array):
    return max_subarray_sum_step(array, 0, len(array))

def max_subarray_sum_step(array, l, r):
    if r - l == 1:
        return array[l]
    mid = int((r + l) / 2)
    max_sum_left = max_subarray_sum_step(array, l, mid)
    max_sum_right = max_subarray_sum_step(array, mid, r)
    max_sum_crossing = max_crossing_sum(array, l, r, mid)
    return max(max_sum_left, max_sum_right, max_sum_crossing)

def max_crossing_sum(array, l, r, mid):
    left_sum = array[mid - 1]
    max_left_sum = left_sum
    for i in range(mid - 2, l - 1, -1):
        left_sum += array[i]
        if left_sum > max_left_sum:
            max_left_sum = left_sum
    right_sum = array[mid]
    max_right_sum = right_sum
    for i in range(mid + 1, r):
        right_sum += array[i]
        if right_sum > max_right_sum:
            max_right_sum = right_sum
    return max_left_sum + max_right_sum

array = list(map(int, input().split()))
print(max_subarray_sum(array))
