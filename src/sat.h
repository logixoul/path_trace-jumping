#pragma once
#include "precompiled.h"
#include "util.h"

template<class T, class SumType>
static void buildSat(Array2D<T>& map, Array2D<SumType>& sums)
{
	const int w = map.w, h = map.h;
	
	sums(0, 0) = map(0, 0);
	for (int y = 1; y < h; y++)
    {
        sums(0, y) = map(0,y) + sums(0,y-1);
    }
    for (int x = 1; x < w; x++)
    {
        sums(x,0) = map(x,0) + sums(x - 1,0);
    }

	int _w = -w;
	int _w_1 = -w-1;
    for (int y = 1; y < h; y++)
    {
		SumType* sumsXY = &sums(1, y);
		T* mapPtr = &map(1, y);
		T* lastMapPtr = &map.data[0]+map.offsetOf(w, y);
		for (;mapPtr < lastMapPtr;)
		{
			*sumsXY = *mapPtr + sumsXY[_w] + sumsXY[-1] - sumsXY[_w_1];
			sumsXY++;
			mapPtr++;
        }
    }
}

template<class T, class SumType>
void satBlur(Array2D<T>& map, int radius)
{
	// the allocation and deallocation makes almost no impact on performance, so we just do it every time.
	Array2D<SumType> sums(map.w, map.h);
	buildSat(map, sums);
	
	const float W = map.w, H = map.h;

    const int diameter = radius * 2 + 1;
	const float sq = diameter * diameter;
	
	const int bottomRight = radius * W + radius, bottomLeft = radius * W - radius - 1;
	const int topRight = -radius * W - W + radius, topLeft = -radius * W - W - radius - 1;
	
    for (int y = radius + 1; y < H - radius; y++)
    {
		SumType* sumsPtr = &sums(radius + 1, y);
		T* newMapPtr = &map(radius + 1, y);
		T* lastNewMapPtr = &map(W - radius, y);
		for (;newMapPtr<lastNewMapPtr;)
		{
            SumType sum = sumsPtr[bottomRight] - sumsPtr[topRight] - sumsPtr[bottomLeft] + sumsPtr[topLeft];

			*newMapPtr = sum / sq;

			sumsPtr++;
			newMapPtr++;
        }
    }
}

/*satBlurEdgeCase<T>(image, radius, newMap, 0,                 radius,             0,                  image.h);
satBlurEdgeCase<T>(image, radius, newMap, image.w - radius,  image.w,            0,                  image.h);
satBlurEdgeCase<T>(image, radius, newMap, radius,            image.w - radius,   0,                  radius);
satBlurEdgeCase<T>(image, radius, newMap, radius,            image.w - radius,   image.h-radius,     image.h);*/
template<class Ti>
void satBlurEdgeCase(Array2D<Ti>& image, int radius, Array2D<Ti>& result, int x1, int x2, int y1, int y2)
{
    Vec2i offset(-radius, -radius);
    for(int x=x1; x<x2; x++) {
        for(int y=y1;y<y2; y++) {
            Ti sum;
            result(x,y) = sum;
        }
    }
}

