// Copyright 2022-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/mem.h>
#include <os/os.h>
#include "lzma.h"

static ELzmaFinishMode lzma_finishmode = LZMA_FINISH_END;
static ELzmaStatus lzma_status = LZMA_STATUS_NOT_SPECIFIED;

void *SzAlloc(ISzAllocPtr p, size_t size)
{
	return os_malloc(size);
}

void SzFree(ISzAllocPtr p, void *address)
{
	p = p;
	os_free(address);
}

void lzma_set_finishmode(ELzmaFinishMode mode)
{
	lzma_finishmode = mode;
}

ELzmaFinishMode lzma_get_finishmode()
{
	return lzma_finishmode;
}

void lzma_set_status(ELzmaStatus status)
{
	lzma_status = status;
}

ELzmaStatus lzma_get_status()
{
	return lzma_status;
}

SRes bk_lzma_decode(Byte *src, SizeT src_len, Byte *dest, SizeT dest_len)
{
	SizeT src_size = src_len - LZMA_PROPS_SIZE;
	SizeT prop_size = LZMA_PROPS_SIZE;
	ISzAlloc alloc = {SzAlloc, SzFree};

	return LzmaDecode(dest, &dest_len, src + LZMA_PROPS_SIZE, &src_size, (const Byte *)src,
					  prop_size, lzma_finishmode, &lzma_status, &alloc);

}
