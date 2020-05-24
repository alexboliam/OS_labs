#include <iostream>
#include <list>
#include <Windows.h>
#include <map>

using namespace std;

class MemPage
{
	struct mem_block
	{
		uint8_t* ptr;
		size_t size;
		mem_block* next;
		mem_block* prev;
	};

public:
	enum class page_status { Free, Splitted, Used };

	MemPage(uint8_t* mem, size_t size)
	{
		ptr_ = mem;
		size_ = size;
		next_page_ = nullptr;
		status_ = page_status::Free;

		used_blocks_ = new list<mem_block*>();
		free_blocks_ = new list<mem_block*>();

		last_block_ = new mem_block();
		last_block_->ptr = mem;
		last_block_->size = size;
		last_block_->next = nullptr;
		last_block_->prev = nullptr;

		free_blocks_->push_back(last_block_);
	}
	void* mem_alloc(size_t size)
	{
		size = align_block_size(size);

		auto block = find_first_block(size);

		if (block == nullptr)
		{
			return nullptr;
		}

		split_block(block, size);

		size_ -= size;

		if (size_ == 0)
		{
			status_ = page_status::Used;
		}
		else
		{
			status_ = page_status::Splitted;
		}

		return block->ptr;
	}
	void* mem_realloc(void* addr, size_t size)
	{
		mem_free(addr);
		return mem_alloc(size);
	}
	void mem_free(void* addr)
	{
		const auto ptr = reinterpret_cast<uint8_t*>(addr);

		for (auto block : *used_blocks_)
		{
			if (block->ptr != ptr) continue;

			size_ += block->size;

			used_blocks_->remove(block);
			const auto joined_block = join_block(block);
			free_blocks_->push_front(joined_block);

			return;
		}

		if (used_blocks_->empty())
		{
			status_ = page_status::Free;
		}
	}
	size_t get_size()
	{
		return size_;
	}
	MemPage* get_next_page()
	{
		return next_page_;
	}
	page_status get_status()
	{
		return status_;
	}
	uint8_t* get_ptr()
	{
		return ptr_;
	}

private:
	uint8_t* ptr_;
	size_t size_;
	MemPage* next_page_;
	page_status status_;

	list<mem_block*>* free_blocks_;
	list<mem_block*>* used_blocks_;
	mem_block* last_block_;

	size_t align_block_size(size_t size)
	{
		return (size + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1);
	}
	mem_block* find_first_block(const size_t size)
	{
		for (auto block : *free_blocks_)
		{
			if (block->size < size) continue;
			return block;
		}

		return nullptr;
	}
	void split_block(mem_block* block, size_t size)
	{
		free_blocks_->remove(block);
		used_blocks_->push_back(block);

		if (block->size == size) return;

		const auto size_diff = block->size - size;

		const auto splitted_block = new mem_block();
		splitted_block->ptr = block->ptr + size;
		splitted_block->size = size_diff;
		splitted_block->prev = block;
		splitted_block->next = block->next;

		block->next = splitted_block;
		block->size = size;

		free_blocks_->push_back(splitted_block);
	}
	void join_blocks(mem_block* left, mem_block* right)
	{
		left->size += right->size;
		left->next = right->next;
		free_blocks_->remove(right);
	}
	mem_block* join_block(mem_block* block)
	{
		if (block->next != nullptr && (block + block->size == block->next) && contains_block(free_blocks_, block->next))
		{
			join_blocks(block->next, block);
		}

		if (block->prev != nullptr && (block->prev + block->prev->size == block) && contains_block(free_blocks_, block->prev))
		{
			join_blocks(block->prev, block);
			return block->prev;
		}

		return block;
	}
	bool contains_block(const list<mem_block*>* list, mem_block* block)
	{
		for (auto list_block : *list)
		{
			if (block == nullptr) continue;
			if (list_block->ptr == block->ptr) return true;
		}

		return false;
	}
};

class MyCppAllocator {

	struct mem_block //структура как тип блока
	{
		unsigned char *ptr; //указатель на область в памяти
		unsigned int size; //размер блока
		mem_block *next; //указатель на предыдущий блок
		mem_block *prev; //указатель на следующий блок 
	};

public:
	MyCppAllocator(size_t page_size)
	{
		page_size_ = page_size;

		page_mems_ = new list<size_t>();
		mem_pages_ = new multimap<size_t, MemPage*>();
		big_mem_pages_ = new list<MemPage*>();

		fill_page_mems(page_mems_);
		auto mem = alloc_heap_mem(10 * page_size);
		make_pages(mem, 10 * page_size);
	}

	void * mem_alloc(size_t size)
	{
		if (size <= page_size_)
		{
			for (auto page_mem : *page_mems_)
			{
				if (page_mem < size) continue;

				auto eqr = mem_pages_->equal_range(page_mem);
				auto
					st = eqr.first,
					en = eqr.second;
				for (auto it = st; it != en; ++it) {
					auto page = it->second;
					if (page->get_size() < size) continue;

					auto mem = page->mem_alloc(size);

					return mem;
				}
			}

			for (auto page : *big_mem_pages_)
			{
				if (page->get_status() != MemPage::page_status::Free) continue;
				big_mem_pages_->remove(page);

				auto mem = page->mem_alloc(size);

				for (auto page_mem : *page_mems_)
				{
					if (page_mem < page->get_size()) continue;

					mem_pages_->insert(pair<size_t, MemPage*>(page_mem, page));
					break;
				}

				return mem;
			}
		}
		else
		{
			auto page_count = align_page_size(size) / page_size_;
			for (auto page : *big_mem_pages_)
			{
				if (page->get_status() != MemPage::page_status::Free) continue;
				{
					auto res = true;
					{

						auto pg = page->get_next_page();
						for (size_t i = 0; i < page_count - 1; ++i)
						{
							if (pg->get_status() != MemPage::page_status::Free)
							{
								res = false;
								break;
							}
						}
					}

					if (!res) continue;
				}

				auto mem = page->mem_alloc(page_size_);
				auto pg = page->get_next_page();
				big_mem_pages_->remove(page);
				for (auto page_mem : *page_mems_)
				{
					if (page_mem < page->get_size()) continue;

					mem_pages_->insert(pair<size_t, MemPage*>(page_mem, page));
					break;
				}
				for (size_t i = 0; i < page_count - 1; ++i)
				{
					pg->mem_alloc(page_size_);
					big_mem_pages_->remove(pg);
					for (auto page_mem : *page_mems_)
					{
						if (page_mem < pg->get_size()) continue;

						mem_pages_->insert(pair<size_t, MemPage*>(page_mem, pg));
						break;
					}
				}

				return mem;
			}
		}

		return nullptr;
	}
	void *mem_realloc(void *addr, unsigned int size) {
		mem_free(addr); //освобождаем память
		return mem_alloc(size); 
	}
	void mem_free(void* addr)
	{
		for (auto page_pair : *mem_pages_)
		{
			auto page = page_pair.second;
			if (addr >= page->get_ptr() && addr <= page->get_ptr())
			{
				page->mem_free(addr);
				if (page->get_status() == MemPage::page_status::Free)
				{
					for (auto it = mem_pages_->begin(); it != mem_pages_->end(); ++it)
					{
						if (it->second == page)
						{
							mem_pages_->erase(it);
							break;
						}
					}
					big_mem_pages_->push_back(page);
				}

				return;
			}
		}

		for (auto page : *big_mem_pages_)
		{
			if (addr >= page->get_ptr() && addr <= page->get_ptr())
			{
				page->mem_free(addr);
				return;
			}
		}
	}

private:
	size_t page_size_;

	list<size_t>* page_mems_;
	multimap<size_t, MemPage*>* mem_pages_;
	list<MemPage*>* big_mem_pages_;

	uint8_t* alloc_heap_mem(size_t size)
	{
		const auto mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);

		if (mem == nullptr)
		{
			return nullptr;
		}

		return static_cast<uint8_t*>(mem);
	}
	size_t align_page_size(size_t size)
	{
		return (size + page_size_ - 1) & ~(page_size_ - 1);
	}
	void fill_page_mems(list<size_t>* page_mems)
	{
		for (size_t mem_size = 1; mem_size <= page_size_; mem_size = mem_size << 1)
		{
			page_mems->push_back(mem_size);
		}
	}
	void make_pages(uint8_t* mem, size_t mem_size)
	{
		auto page_count = align_page_size(mem_size) / page_size_;
		auto cur_mem = mem;
		for (size_t i = 0; i < page_count; ++i)
		{
			big_mem_pages_->push_back(new MemPage(cur_mem, page_size_));
			cur_mem += page_size_;
		}
	}

	unsigned int align_size(unsigned int size)
	{
		return (size + sizeof(int) - 1) & ~(sizeof(int) - 1);
	}
};


int main() {
	auto allocator = new MyCppAllocator(2048);

	cout << "Create two blobs with int values:" << endl;
	auto mem1 = allocator->mem_alloc(sizeof(int));
	auto mem2 = allocator->mem_alloc(sizeof(int));
	auto var1 = new (mem1) int(1);
	auto var2 = new (mem2) int(2);

	cout << "blob 1 on mem 1: " << var1 << endl;
	cout << "blob 2 on mem 2: " << var1 << endl;

	cout << "-------------------------" << endl;

	cout << "Realloc mem 1 to mem 3. Expect to get 3rd mem with freed mem 1 address." << endl;
	auto mem3 = allocator->mem_realloc(mem1, sizeof(int));
	
	auto blob3 = new (mem3) int(3);
	cout << "Create blob3 on mem 3." << endl;

	cout << "-------------------------" << endl;

	cout << "Blob 1 that points to mem 1 expected to be blob 3 as mem 3 expected to have address of mem 1 " << endl;
	cout << "Blob 1 on mem 1 (mem 3): " << var1 << endl << endl;

	system("pause");
}