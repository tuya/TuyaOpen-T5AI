/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdint.h>
#include <math.h>
#include <new>

#if OPTION_SCPI
#include <scpi/scpi.h>
#endif

namespace eez {

void initAllocHeap(uint8_t *heap, size_t heapSize);

void *alloc(size_t size, uint32_t id);
void free(void *ptr);

template<class T> struct ObjectAllocator {
	static T *allocate(uint32_t id) {
		auto ptr = alloc(sizeof(T), id);
		return new (ptr) T;
	}
	static void deallocate(T* ptr) {
		ptr->~T();
		free(ptr);
	}
};

#if OPTION_SCPI
void dumpAlloc(scpi_t *context);
#endif

void getAllocInfo(uint32_t &free, uint32_t &alloc);

} // eez
