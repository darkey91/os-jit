#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

const size_t MAX_CODE_LEN = 4096 * 2, PAGE_SIZE = 4096;

int get_byte(unsigned char c) {
    if (c >= '0'  && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    }

    if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    }
    return 0;
}

unsigned char get_byte(unsigned char first,unsigned char second) {
    unsigned char f = static_cast<unsigned char>(get_byte(first));
    unsigned char s = static_cast<unsigned char>(get_byte(second));
    f = (f << 4);
    return f | s;
}

void type_int(unsigned char *dest, size_t &index, const std::string hex_number) {
    for (int i = hex_number.size() - 1; i >= 0; i -= 2) {
        dest[++index] = get_byte(hex_number[i - 1], hex_number[i]);
    }
}

void usage_msg() {
    std::cout <<  " \nCommands: c [index] [new_value] - to change\n e - to evaluate sum\n";;
}

void info() {
    std::cout << " Simple program counts sum of integer values from stdin. \nLeft numbers are indexes of addendums. \n0 - to stop reading. After reading it is possible to change addenddum."
                 " \nCommands:\n c [index] [new_value] - to change\n e - to evaluate sum\n";
}

void type_head(unsigned char *code, size_t &size, const std::string &default_hex_value) {
    size = 0;
    code[size] = 0x55;

    code[++size] =0x48;
    code[++size] =0x89;
    code[++size] =0xe5;

    code[++size] = 0xc7;
    code[++size] = 0x45;
    code[++size] = 0xfc;
    type_int(code, size, default_hex_value);
}

void type_end(unsigned char *code, size_t &size) {
    code[++size] = 0x8b;
    code[++size] = 0x45;
    code[++size] = 0xfc;
    code[++size] = 0x5d;
    code[++size] = 0xc3;

    ++size;
}

void clean(void *mem, size_t size) {
    if (munmap(mem, size) == -1) {
        std::cerr << "Can't free memory.  " << strerror(errno);
    }
}

int main() {
    info();
    size_t size = 0, index = 0;
    unsigned char code[MAX_CODE_LEN];

    char command;
    std::stringstream ss;
    std::string hex_number, default_hex_value = "00000000";
    int result = 0, value;
    bool finish = false;

    type_head(code, size, default_hex_value);

    while (true) {
        std::cout << ++index << ":\t";
        std::cin >> value;
        if (value == 0) break;
        result += value;

        ss.clear();
        ss << std::setfill('0') << std::setw(sizeof(int) * 2) <<  std::hex << (value < 0 ? value * -1 : value);
        ss >> hex_number;

        code[++size] = 0x81;

        if (value > 0) {
            //add
            code[++size] = 0x45;
        } else {
            code[++size] = 0x6d;
        }
        code[++size] = 0xfc;
        type_int(code, size, hex_number);
    }

    type_end(code, size);

    auto *mem = mmap(nullptr, size, PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);

    if (mem == MAP_FAILED) {
        std::cerr << "Can not allocate memory. " << strerror(errno) << "\n";
        return 1;
    }
    memcpy(mem, code, size);

    int (*sum)() = reinterpret_cast<int (*)()>(mem);

    std::cout << "Answer must be " << result << "\n";
    std::cout << "Answer is " << sum() << "\n";

    while (std::cin >> command && !finish) {
        switch (command) {
        case 'c' : {
            size_t k;
            //change k-th addendum
            std::cin >> k;
            if (k > 0 && k < index) {
                if (mprotect(mem, size, PROT_READ | PROT_WRITE) == -1) {
                    std::cerr << "Can't change protection mode. \n" << strerror(errno) << "\n";
                    finish = true;
                } else {
                    k = 14 + 7 * (k - 1);
                    std::cin >> value;

                    ss.clear();
                    ss << std::setfill('0') << std::setw(sizeof(int) * 2) <<  std::hex << (value < 0 ? value * -1 : value);
                    ss >> hex_number;

                    if (value < 0) {
                        reinterpret_cast<unsigned char *>(mem)[k - 2] = 0x6d;
                    }
                    --k;
                    type_int(reinterpret_cast<unsigned char *>(mem), k, hex_number);
                }
            }
            else
                std::cout << "Index of addendum must be greater than 0 and less than " << index << ".\nTry again\n";
            break;
        }
        case 'e': {
            if (mprotect(mem, size, PROT_READ | PROT_EXEC) == -1) {
                std::cerr << "Can't change protection mode. \n" << strerror(errno) << "\n";
                finish = true;
            } else {
                int (*sum)() = reinterpret_cast<int (*)()>(mem);
                std::cout << "Answer is " << sum() << "\n";
            }
            break;
        }
        case '!': {
            //finish
            finish = true;
            break;
        }
        default: {
            usage_msg();
        }
        }
    }

    return 0;
}
	 
