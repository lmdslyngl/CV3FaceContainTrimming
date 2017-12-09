#pragma once

#include <algorithm>

// 最小値と最大値を指定して正規化
template <class T> T Normalize(T value, T minValue, T maxValue) {
    return (value - minValue) / (maxValue - minValue);
}

// minValueからmaxValueに値を押し込める
template <class T> T Clamp(T value, T minValue, T maxValue) {
    return std::max(std::min(maxValue, value), minValue);
}

// 長さを持った領域をminValueからmaxValueに押し込める
template <class T> T RangedClamp(T start, T length, T minValue, T maxValue) {
    if( start < 0 ) {
        start = 0;
    } else if( maxValue <= start + (length - 1) ) {
        start = maxValue - (length - 1);
    }
    return start;
}

// リストを正規化
template <class InputIterator> void NormalizeList(
    InputIterator first, InputIterator last)
{
    auto minmax = std::minmax_element(first, last);
    auto minValue = *minmax.first;
    auto maxValue = *minmax.second;
    std::for_each(first, last,
        [minValue, maxValue](auto &v) {
            v = Normalize(v, minValue, maxValue);
        });
}

