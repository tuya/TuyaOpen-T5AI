//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <common/bk_assert.h>

#include <driver/int.h>

#include <driver/psram.h>
#include "frame_buffer_mapping.h"
#include "psram_mem_slab.h"

#define TAG "psram_fb"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define FB_LIST_PATTERN           (0xA55A)
#define FB_ALLOCATED_PATTERN      (0x8338)
#define FB_FREE_PATTERN           (0xF00F)

fb_mem_heap_t frame_mem_heap = {0};

static void frame_buffer_heap_init(uint8_t type, uint8_t* heap, uint32_t heap_size)
{
	// align first free descriptor to word boundary
	frame_mem_heap.heap[type] = (struct fb_block_free *)heap;

	// initialize the first block
	// compute the size from the last aligned word before heap_end
	frame_mem_heap.heap[type]->free_size = ((uint32_t)&heap[heap_size] & (~3)) - (uint32_t)(frame_mem_heap.heap[type]);
	frame_mem_heap.heap[type]->corrupt_check = FB_LIST_PATTERN;
	frame_mem_heap.heap[type]->next = NULL;
	frame_mem_heap.heap[type]->previous = NULL;
	frame_mem_heap.heap_size[type] = heap_size;
	LOGD("%s heap:%p, type %d size %d\n", __func__, heap, type, heap_size);
	LOGD("%s, free_size:%d, check:0x%x\r\n", __func__, frame_mem_heap.heap[type]->free_size, frame_mem_heap.heap[type]->corrupt_check);
}

/**
 * Check if memory pointer is within heap address range
 *
 * @param[in] type Memory type.
 * @param[in] mem_ptr Memory pointer
 * @return True if it's in memory heap, False else.
 */
static bool frame_buffer_mem_is_in_heap(uint8_t type, void* mem_ptr)
{
	bool ret = false;
	uint8_t* block = (uint8_t*)frame_mem_heap.heap[type];
	uint32_t size = frame_mem_heap.heap_size[type];

	if((((uint32_t)mem_ptr) >= ((uint32_t)block))
		&& (((uint32_t)mem_ptr) <= (((uint32_t)block) + size)))
	{
		ret = true;
	}

	return ret;
}

void bk_psram_frame_buffer_init(void)
{
	uint32_t end_adderss = PSRAM_ADDR;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	end_adderss += sizeof(psram_mem_slab);

	if (end_adderss > CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER)
	{
		LOGE("multimedia allocte psram over the maximum length!!!\n");
		BK_ASSERT(0);
	}

	os_memset(frame_mem_heap.heap, 0, sizeof(struct fb_block_free*) * PSRAM_HEAP_MAX);
	os_memset(frame_mem_heap.heap_size, 0, sizeof(uint32_t) * PSRAM_HEAP_MAX);

	if (PSRAM_MEM_SLAB_USER_SIZE)
	{
		frame_buffer_heap_init(PSRAM_HEAP_USER, &psram_mem_slab->user[0], PSRAM_MEM_SLAB_USER_SIZE);
	}

	if (PSRAM_MEM_SLAB_AUDIO_SIZE)
	{
		frame_buffer_heap_init(PSRAM_HEAP_AUDIO, &psram_mem_slab->audio[0], PSRAM_MEM_SLAB_AUDIO_SIZE);
	}

	if (PSRAM_MEM_SLAB_ENCODE_SIZE)
	{
		frame_buffer_heap_init(PSRAM_HEAP_ENCODE, &psram_mem_slab->encode[0], PSRAM_MEM_SLAB_ENCODE_SIZE);
	}

	if (PSRAM_MEM_SLAB_DISPLAY_SIZE)
	{
		frame_buffer_heap_init(PSRAM_HEAP_YUV, &psram_mem_slab->display[0], PSRAM_MEM_SLAB_DISPLAY_SIZE);
	}

	GLOBAL_INT_RESTORE();
}

void *bk_psram_frame_buffer_malloc(psram_heap_type_t type, uint32_t size)
{
	struct fb_block_free *node = NULL,*found = NULL;
	uint8_t cursor = 0;
	fb_block_used *alloc = NULL;
	uint32_t totalsize;

	if (frame_mem_heap.heap_size[type] == 0)
	{
		LOGE("%s, type:%d not init\r\n", __func__, type);
		BK_ASSERT(0);
	}

	GLOBAL_INT_DECLARATION();

	// compute overall block size (including requested size PLUS descriptor size)
	// aligned 4 byte
	if (size & 0x3)
	{
		size = ((size >> 2) + 1) << 2;
	}
	totalsize = size + sizeof(fb_block_used);
	if(totalsize < sizeof(struct fb_block_free))
	{
		totalsize = sizeof(struct fb_block_free);
	}

	// sanity check: the totalsize should be large enough to hold free block descriptor
	BK_ASSERT(totalsize >= sizeof(struct fb_block_free));

	// protect accesses to descriptors
	GLOBAL_INT_DISABLE();

	 while((cursor < PSRAM_HEAP_MAX)&& (found == NULL))
	{
		uint8_t heap_id = COMMON_MOD((cursor + type), PSRAM_HEAP_MAX);

		// Select Heap to use, first try to use current heap.
		node = frame_mem_heap.heap[heap_id];
		BK_ASSERT(node != NULL);

		// go through free memory blocks list
		while (node != NULL)
		{
			BK_ASSERT(node->corrupt_check == FB_LIST_PATTERN);

			// check if there is enough room in this free block
			if (node->free_size >= (totalsize))
			{
				if ((node->free_size >= (totalsize + sizeof(struct fb_block_free)))
						|| (node->previous != NULL))
				{
					// if a match was already found, check if this one is smaller
					if ((found == NULL) || (found->free_size > node->free_size))
					{
						found = node;
					}
				}
			}

			// move to next block
			node = node->next;
		}

		// Update size to use complete list if possible.
		if(found != NULL)
		{
			if (found->free_size < (totalsize + sizeof(struct fb_block_free)))
			{
				totalsize = found->free_size;
			}
		}

		// increment cursor
		cursor++;
	}

	//BT_ASSERT_INFO(found != NULL, size, type);
	// Re-boot platform if no more empty space
	if(found == NULL)
	{
		//platform_reset(RESET_MEM_ALLOC_FAIL);
		GLOBAL_INT_RESTORE();
		return NULL;
	}
	 else
	{
		// DBG_MEM_GRANT_CTRL(found, true);
		// sublist completely reused
		if (found->free_size == totalsize)
		{
			BK_ASSERT(found->previous != NULL);
			// update double linked list
			found->previous->next = found->next;
			if(found->next != NULL)
			{
				found->next->previous = found->previous;
			}

			// compute the pointer to the beginning of the free space
			alloc = (fb_block_used*) ((uint32_t)found);
		}
		else
		{
			// found a free block that matches, subtract the allocation size from the
			// free block size. If equal, the free block will be kept with 0 size... but
			// moving it out of the linked list is too much work.
			found->free_size -= totalsize;

			// compute the pointer to the beginning of the free space
			alloc = (fb_block_used*) ((uint32_t)found + found->free_size);
		}

		//TRC_REQ_MEM_ALLOC(trc_heap_id, alloc, size);

		// save the size of the allocated block
		alloc->size = totalsize;
		alloc->corrupt_check = FB_ALLOCATED_PATTERN;
		// move to the user memory space
		alloc++;
	}

	// end of protection (as early as possible)
	GLOBAL_INT_RESTORE();
	//BK_ASSERT(node == NULL);

	return (void*)alloc;
}


void bk_psram_frame_buffer_free(void* mem_ptr)
{
	struct fb_block_free *freed;
	fb_block_used *bfreed;
	struct fb_block_free *node, *next_node, *prev_node;
	uint32_t size;
	uint8_t cursor = 0;
	GLOBAL_INT_DECLARATION();

	// sanity checks
	if (mem_ptr == NULL)
		return;

	//debug_mem_reset((uint32_t*)mem_ptr);
	// point to the block descriptor (before user memory so decrement)
	bfreed = ((fb_block_used *)mem_ptr) - 1;

	// check if memory block has been corrupted or not
	//BT_ASSERT_INFO(bfreed->corrupt_check == FB_ALLOCATED_PATTERN, bfreed->corrupt_check, mem_ptr);
	// change corruption token in order to know if buffer has been already freed.
	bfreed->corrupt_check = FB_FREE_PATTERN;

	// point to the first node of the free elements linked list
	size = bfreed->size;
	node = NULL;

	freed = ((struct fb_block_free *)bfreed);

	//DBG_MEM_PERM_SET(bfreed, sizeof(struct fb_block_used), false, false, false);

	// protect accesses to descriptors
	GLOBAL_INT_DISABLE();
	//DBG_MEM_GRANT_CTRL(mem_ptr, true);

	// Retrieve where memory block comes from
	while(((cursor < PSRAM_HEAP_MAX)) && (node == NULL)) {
		if(frame_buffer_mem_is_in_heap(cursor, mem_ptr))
		{
			// Select Heap to use, first try to use current heap.
			node = frame_mem_heap.heap[cursor];
		}
		else
		{
			cursor ++;
		}
	}

	// sanity checks
	BK_ASSERT(node != NULL);
	BK_ASSERT(((uint32_t)mem_ptr > (uint32_t)node));

	//TRC_REQ_MEM_FREE(trc_heap_id, freed, size);
	//DBG_MEM_PERM_SET(freed, size, false, false, false);

	prev_node = NULL;

	while(node != NULL)
	{
		BK_ASSERT(node->corrupt_check == FB_LIST_PATTERN);
		// check if the freed block is right after the current block
		if ((uint32_t)freed == ((uint32_t)node + node->free_size))
		{
			// append the freed block to the current one
			node->free_size += size;

			// check if this merge made the link between free blocks
			if (((uint32_t) node->next) == (((uint32_t)node) + node->free_size))
			{
				next_node = node->next;
				// add the size of the next node to the current node
				node->free_size += next_node->free_size;
				// update the next of the current node
				BK_ASSERT(next_node != NULL);
				node->next = next_node->next;
				// update linked list.
				if(next_node->next != NULL)
				{
					next_node->next->previous = node;
				}
			}
			goto free_end;
		}
		else if ((uint32_t)freed < (uint32_t)node)
		{
			// sanity check: can not happen before first node
			BK_ASSERT(prev_node != NULL);

			// update the next pointer of the previous node
			prev_node->next = freed;
			freed->previous = prev_node;

			freed->corrupt_check = FB_LIST_PATTERN;

			// check if the released node is right before the free block
			if (((uint32_t)freed + size) == (uint32_t)node)
			{
				// merge the two nodes
				freed->next = node->next;
				if(node->next != NULL)
				{
					node->next->previous = freed;
				}
				freed->free_size = node->free_size + size;
			}
			else
			{
				// insert the new node
				freed->next = node;
				node->previous = freed;
				freed->free_size = size;
			}
			goto free_end;
		}

		// move to the next free block node
		prev_node = node;
		node = node->next;

	}

	freed->corrupt_check = FB_LIST_PATTERN;

	// if reached here, freed block is after last free block and not contiguous
	prev_node->next = (struct fb_block_free*)freed;
	freed->next = NULL;
	freed->previous = prev_node;
	freed->free_size = size;
	freed->corrupt_check = FB_LIST_PATTERN;


free_end:
	// end of protection
	GLOBAL_INT_RESTORE();
}

