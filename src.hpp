
/**
 * Get specified 4096 * n bytes from the memory.
 * @param n
 * @return the address of the block
 */
int* getNewBlock(int n);

/**
 * Free specified 4096 * n bytes from the memory.
 * @param block the pointer to the block
 * @param n
 */
void freeBlock(const int* block, int n);

#include <vector>
#include <unordered_map>
#include <algorithm>

class Allocator {
public:
    Allocator() {
        // Initialize any data structures needed
        currentBlock = nullptr;
        currentOffset = 0;
        blockSize = 0;
    }

    ~Allocator() {
        // Free all allocated blocks
        for (auto& blockInfo : allocatedBlocks) {
            freeBlock(blockInfo.first, blockInfo.second);
        }
        allocatedBlocks.clear();
        
        // Free current block if it exists and hasn't been added to allocatedBlocks
        if (currentBlock != nullptr && allocatedBlocks.find(currentBlock) == allocatedBlocks.end()) {
            freeBlock(currentBlock, blockSize);
        }
    }

    /**
     * Allocate a sequence of memory space of n int.
     * @param n
     * @return the pointer to the allocated memory space
     */
    int* allocate(int n) {
        // Calculate required space in integers
        int requiredInts = n;
        
        // Calculate how many 4096-byte blocks we need
        // Each block has 4096/sizeof(int) integers
        int intsPerBlock = 4096 / sizeof(int);
        int blocksNeeded = (requiredInts + intsPerBlock - 1) / intsPerBlock; // Ceiling division
        
        // First, try to allocate from the current block if it has enough space
        if (currentBlock != nullptr && (currentOffset + requiredInts) <= blockSize * intsPerBlock) {
            int* result = currentBlock + currentOffset;
            currentOffset += requiredInts;
            return result;
        }
        
        // If we can't use the current block, look for a freed block that can fit
        for (auto it = freeList.begin(); it != freeList.end(); ++it) {
            int* block = it->first;
            int blockInts = it->second * intsPerBlock;
            int blockOffset = 0;
            
            // Check if this freed block can fit our allocation
            if (requiredInts <= blockInts) {
                // Use this freed block
                int* result = block + blockOffset;
                // Update the free list - reduce the size or remove if fully used
                if (requiredInts == blockInts) {
                    // Fully used, remove from free list
                    freeList.erase(it);
                } else {
                    // Partially used, update the remaining size
                    int remainingBlocks = (blockInts - requiredInts + intsPerBlock - 1) / intsPerBlock;
                    freeList[block] = remainingBlocks;
                }
                
                // Add to allocated blocks
                allocatedBlocks[result] = std::make_pair(block, n);
                return result;
            }
        }
        
        // If no suitable freed block, allocate a new block
        currentBlock = getNewBlock(blocksNeeded);
        if (currentBlock == nullptr) {
            return nullptr; // Allocation failed
        }
        
        blockSize = blocksNeeded;
        currentOffset = requiredInts;
        
        // Add to allocated blocks
        allocatedBlocks[currentBlock] = std::make_pair(currentBlock, n);
        
        return currentBlock;
    }

    /**
     * Deallocate the memory that is allocated by the allocate member
     * function. If n is not the number that is called when allocating,
     * the behaviour is undefined.
     */
    void deallocate(int* pointer, int n) {
        // Find which block this pointer belongs to
        auto it = allocatedBlocks.find(pointer);
        if (it == allocatedBlocks.end()) {
            // Pointer not found in allocated blocks
            return;
        }
        
        // Get the block info
        int* block = it->second.first;
        int blockSize = it->second.second;
        
        // Remove from allocated blocks
        allocatedBlocks.erase(it);
        
        // Add to free list
        int intsPerBlock = 4096 / sizeof(int);
        int blocksForN = (n + intsPerBlock - 1) / intsPerBlock; // Blocks needed for n integers
        
        // If this deallocation is at the end of the current block, we can update currentOffset
        if (block == currentBlock && pointer == (currentBlock + currentOffset - n)) {
            currentOffset -= n;
            // If the entire current block is now free, we can free it
            if (currentOffset == 0) {
                freeBlock(currentBlock, blockSize);
                currentBlock = nullptr;
                this->blockSize = 0;
            }
            return;
        }
        
        // Otherwise, add to free list
        freeList[pointer] = blocksForN;
    }

private:
    // Current block being used for allocation
    int* currentBlock;
    int currentOffset;  // Current offset in the current block (in integers)
    int blockSize;      // Size of current block in number of 4096-byte blocks
    
    // Track allocated blocks and their original block info
    std::unordered_map<int*, std::pair<int*, int>> allocatedBlocks;
    
    // Free list - track freed blocks that can be reused
    std::unordered_map<int*, int> freeList;  // pointer to block, number of 4096-byte blocks
};
