#ifndef MMU__HEADER
#define MMU__HEADER

unsigned int mmu_read_byte(unsigned int address);
unsigned int mmu_read_word(unsigned int address);
unsigned int mmu_read_long(unsigned int address);
void mmu_write_byte(unsigned int address, unsigned int value);
void mmu_write_word(unsigned int address, unsigned int value);
void mmu_write_long(unsigned int address, unsigned int value);

#endif /* MMU__HEADER */
