#include <iostream>
#include <list>
#include <Windows.h>

using namespace std;

class MyCppAllocator {

	struct mem_block // тип блока
	{
		unsigned char *ptr; // указатель на область в памяти
		unsigned int size; // размер блока
		mem_block *next; // указатель на предыдущий блок
		mem_block *prev; // указатель на следующий блок 
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

		size = align_size(size); // выравниваем размер блока кратным 4 байтам

		mem_block *block = find_free_block(size); // ищем свободный блок подходящего размера

		if (block == nullptr) // если такой не нашелся, создаем новый
		{
			block = alloc_block(size);

			if (block == nullptr)
			{
				return nullptr;
			}
		}

		split_block(block, size); // делаем блок занятым, если блок был взят из свободных и его можно обрезать, делаем это

		return block->ptr; // возвращаем указатель на наши данные в памяти
	}
	void *mem_realloc(void *addr, unsigned int size) {
		mem_free(addr); // освобождаем память
		return mem_alloc(size); 
	}
	void mem_free(void *addr) {
		const auto ptr = reinterpret_cast<uint8_t*>(addr); // приводим указатель к сравнимому типу

		for (auto block : *used_blocks) // ищем блок, освобождаем его, приклеиваем к соседнему
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
	list<mem_block*> *free_blocks; // список свободных блоков
	list<mem_block*> *used_blocks; // список занятых блоков
	mem_block *first_block; // указатель на первый блок в выделенной области памяти
	mem_block *last_block; // указатель на последний блок в выделенной области памяти

	unsigned int align_size(unsigned int size)
	{
		return (size + sizeof(int) - 1) & ~(sizeof(int) - 1);
	}
	mem_block *find_free_block(unsigned int size) {
		for (mem_block *block : *free_blocks) //ищем первый подходящий блок из списка свободных
		{
			if (block->size < size) 
				continue;
			return block;
		}

		return nullptr; //если не нашли, возвращаем nullptr
	}
	mem_block *alloc_block(unsigned int size) {
		const auto mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size); // выделяем память под блок из ОС

		if (mem == nullptr)
		{
			return nullptr;
		}

		const auto block = new mem_block(); // создаем и инициализируем блок
		block->next = nullptr;
		block->prev = nullptr;
		block->ptr = static_cast<uint8_t*>(mem); 
		block->size = size;

		if ((free_blocks->size() == 0) && (used_blocks->size() == 0)) // если это самый первый блок, помечаем его (для корректного выведения состояния памяти)
			first_block = block;

		free_blocks->push_back(block); // записываем блок в список свободных

		if (last_block != nullptr) // делаем этот блок хвостом области
		{
			last_block->next = block;
			block->prev = last_block;
		}

		last_block = block;

		return block;
	}
	void split_block(mem_block *block, unsigned int size) {
		free_blocks->remove(block); // убираем блок из списка свободных
		used_blocks->push_back(block); // вставляем блок в список занятых

		if (block->size == size) return; // если размер блока равен данным, возвращаемся

		const auto size_diff = block->size - size; // считаем разницу размеров

		const auto splitted_block = new mem_block(); // обрезая, создаем еще один блок
		splitted_block->ptr = block->ptr + size;
		splitted_block->size = size_diff;
		splitted_block->prev = block;
		splitted_block->next = block->next;

		block->next = splitted_block; // связываем их
		block->size = size;

		free_blocks->push_back(splitted_block); // обрезка остается свободной
	}
	void join_block(mem_block *block) {
		// если предыдуший блок свободен, объединяем
		if (block->prev != nullptr && (block->prev + block->prev->size == block) && contains_block(free_blocks, block->prev)) 
		{
			block = join_blocks(block->prev, block);
		}
		// если следующий блок свободен, объединяем
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

		for (auto list_block : *list)// проверяем элементы списка на совпадение
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