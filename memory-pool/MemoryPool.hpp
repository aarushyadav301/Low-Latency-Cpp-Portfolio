template <typename T, size_t blockNum>
class MemoryPool {
    private:
        alignas(T) union PoolBlock {
            T objectData;
            PoolBlock *nextFree;
        };

        PoolBlock freeList[blockNum];
        PoolBlock *head;

    public:
        MemoryPool() {
            for (size_t i = 0; i < blockNum - 1; i++) {
                freeList[i].nextFree = &freeList[i + 1];
            }
            freeList[blockNum - 1].nextFree = nullptr;

            head = &freeList[0];
        }

        T* allocate() {
            if (head == nullptr) {
                return (nullptr);
            }

            PoolBlock *prev = head;
            head = head->nextFree;

            return (reinterpret_cast<T*>(prev));
        }

        void deallocate (T *ptr) {
            PoolBlock *deall = reinterpret_cast<PoolBlock*>(ptr);
            deall->nextFree = head;
            head = deall;
        }
};