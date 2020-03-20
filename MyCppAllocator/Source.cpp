#include <iostream>
#include <list>
#include <Windows.h>

using namespace std;

class MyCppAllocator {

	struct mem_block // ��� �����
	{
		unsigned char *ptr; // ��������� �� ������� � ������
		unsigned int size; // ������ �����
		mem_block *next; // ��������� �� ���������� ����
		mem_block *prev; // ��������� �� ��������� ���� 
	};

public:
	MyCppAllocator()
	{
		used_blocks = new std::list<mem_block*>();
		free_blocks = new std::list<mem_block*>();
		first_block = nullptr;
		last_block = nullptr;
	}

	void mem_dump() {
		cout << "Current state of memory:" << endl;

		if (first_block == nullptr) {
			cout << "Memory is empty" << endl;
			return;
		}

		auto curr_block = first_block;
		while (true) {

			cout << "- Size: " << curr_block->size << " - Status: ";

			if (contains_block(free_blocks, curr_block))
				cout << "free" << endl;
			else if (contains_block(used_blocks, curr_block))
				cout << "being used" << endl;
			else
				cout << "error" << endl;

			if (curr_block->next == nullptr)
				break;

			curr_block = curr_block->next;
		}
	}
	void *mem_alloc(unsigned int size) {

		size = align_size(size); // ����������� ������ ����� ������� 4 ������

		mem_block *block = find_free_block(size); // ���� ��������� ���� ����������� �������

		if (block == nullptr) // ���� ����� �� �������, ������� �����
		{
			block = alloc_block(size);

			if (block == nullptr)
			{
				return nullptr;
			}
		}

		split_block(block, size); // ������ ���� �������, ���� ���� ��� ���� �� ��������� � ��� ����� ��������, ������ ���

		return block->ptr; // ���������� ��������� �� ���� ������ � ������
	}
	void *mem_realloc(void *addr, unsigned int size) {
		mem_free(addr); // ����������� ������
		return mem_alloc(size); 
	}
	void mem_free(void *addr) {
		const auto ptr = reinterpret_cast<uint8_t*>(addr); // �������� ��������� � ���������� ����

		for (auto block : *used_blocks) // ���� ����, ����������� ���, ����������� � ���������
		{
			if (block->ptr != ptr) 
				continue;

			used_blocks->remove(block);
			free_blocks->push_back(block);
			join_block(block);
			return;
		}
	}

private:
	list<mem_block*> *free_blocks; // ������ ��������� ������
	list<mem_block*> *used_blocks; // ������ ������� ������
	mem_block *first_block; // ��������� �� ������ ���� � ���������� ������� ������
	mem_block *last_block; // ��������� �� ��������� ���� � ���������� ������� ������

	unsigned int align_size(unsigned int size)
	{
		return (size + sizeof(int) - 1) & ~(sizeof(int) - 1);
	}
	mem_block *find_free_block(unsigned int size) {
		for (mem_block *block : *free_blocks) //���� ������ ���������� ���� �� ������ ���������
		{
			if (block->size < size) 
				continue;
			return block;
		}

		return nullptr; //���� �� �����, ���������� nullptr
	}
	mem_block *alloc_block(unsigned int size) {
		const auto mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size); // �������� ������ ��� ���� �� ��

		if (mem == nullptr)
		{
			return nullptr;
		}

		const auto block = new mem_block(); // ������� � �������������� ����
		block->next = nullptr;
		block->prev = nullptr;
		block->ptr = static_cast<uint8_t*>(mem); 
		block->size = size;

		if ((free_blocks->size() == 0) && (used_blocks->size() == 0)) // ���� ��� ����� ������ ����, �������� ��� (��� ����������� ��������� ��������� ������)
			first_block = block;

		free_blocks->push_back(block); // ���������� ���� � ������ ���������

		if (last_block != nullptr) // ������ ���� ���� ������� �������
		{
			last_block->next = block;
			block->prev = last_block;
		}

		last_block = block;

		return block;
	}
	void split_block(mem_block *block, unsigned int size) {
		free_blocks->remove(block); // ������� ���� �� ������ ���������
		used_blocks->push_back(block); // ��������� ���� � ������ �������

		if (block->size == size) return; // ���� ������ ����� ����� ������, ������������

		const auto size_diff = block->size - size; // ������� ������� ��������

		const auto splitted_block = new mem_block(); // �������, ������� ��� ���� ����
		splitted_block->ptr = block->ptr + size;
		splitted_block->size = size_diff;
		splitted_block->prev = block;
		splitted_block->next = block->next;

		block->next = splitted_block; // ��������� ��
		block->size = size;

		free_blocks->push_back(splitted_block); // ������� �������� ���������
	}
	void join_block(mem_block *block) {
		// ���� ���������� ���� ��������, ����������
		if (block->prev != nullptr && (block->prev + block->prev->size == block) && contains_block(free_blocks, block->prev)) 
		{
			block = join_blocks(block->prev, block);
		}
		// ���� ��������� ���� ��������, ����������
		if (block->next != nullptr && (block + block->size == block->next) && contains_block(free_blocks, block->next))
		{
			block = join_blocks(block, block->next);
		}
	}
	mem_block *join_blocks(mem_block *left, mem_block *right) {
		left->size += right->size;
		left->next = right->next;
		return left;
	}
	bool contains_block(list<mem_block*> *list, mem_block *block) {
		if (block == nullptr)
			return false;

		for (auto list_block : *list)// ��������� �������� ������ �� ����������
		{
			if (list_block->ptr == block->ptr) 
				return true;
		}
		return false;
	}
};


int main() {
	auto allocator = new MyCppAllocator();

	cout << "Create two blocks with int values" << endl;
	auto mem1 = allocator->mem_alloc(sizeof(int));
	auto mem2 = allocator->mem_alloc(sizeof(int));
	auto var1 = new (mem1) int(1);
	auto var2 = new (mem2) int(2);
	allocator->mem_dump();

	cout << endl << "Realloc mem2 to custom size of 5. Expect to get 3rd block with size of 8 (align to 4x): " << endl;
	auto mem3 = allocator->mem_realloc(mem2, 5);
	allocator->mem_dump();

	cout << endl << "Free block 1" << endl;
	allocator->mem_free(mem1);
	allocator->mem_dump();

	system("pause");
}