#include <stdio.h>
#include <variant>
#include <vector>
#include <inttypes.h>
#include <map>
#include <string>
#include <set>
#include <optional>
#include <sstream>
#include <iostream>




std::map<std::string, uint8_t> register_names = {
    { "A", 0xAA },
    { "B", 0xBB },
    { "C", 0xCC },
    { "D", 0xDD },

    { "CA", 0xCA },
    { "CB", 0xCB },

    { "PC", 0x8c },
    { "MAR", 0x8a },
    { "MDR", 0x8d },
};


std::map<std::string, uint8_t> instrsuction_names = {
    { "NOOP",   0xff },
    { "HALT",   0x00 },
    { "MVI",    0x01 },
    { "MOV",    0x02 },
    { "ADD",    0x03 },
    { "ADDI",   0x04 },
    { "SHFL",   0x05 },
    { "SHFR",   0x06 },
    { "SUB",    0x07 },
    { "SUBI",   0x08 },
    { "LDA",    0x20 },
    { "STA",    0x21 },
    { "JMP",    0x40 },
    { "JMPZ",   0x41 },
    { "JMPE",   0x42 },
    { "JMPI",   0x43 },
    { "JMPR",   0x44 },
    { "JMPL",   0x43 },
    { "JMPS",   0x43 },
    { "OUT",    0xe0 },
    { "OUTI",   0xe1 },
    { "IN",     0xf0 },
    { "INV",    0xf1 },
};




template<typename T2, typename T1>
inline T2 lexical_cast(const T1 &in) {
    T2 out;
    std::stringstream ss;
    ss << in;
    ss >> out;
    return out;
}


class Eater {
private:

    char nextc;

public:

    FILE* file;

    Eater() {}

    void init(FILE* file) {
        this->file = file;
        next();
    }

    char peek() {
        return nextc;
    }

    char next() {
        char c = nextc;
        nextc = getc(file);
        return c;
    }

    bool check(char c) {
        return nextc == c;
    }

    bool eat(char c) {
        if(nextc != c) return false;

        next();
        return true;
    }

    // Special functions

    bool skipSeparator() {
        bool gotSeparator = false;
        if(nextc == ' ' || nextc == '\t') {
            gotSeparator = true;
        }

        while(nextc == ' ' || nextc == '\t') {
            next();
        }

        return gotSeparator;
    }

    bool isSeparator() {
        return (nextc == ' ' || nextc == '\t');
    }

    bool skipWhitespace() {
        bool gotWhitespace = false;
        if(nextc == ' ' || nextc == '\t' || nextc == '\n') {
            gotWhitespace = true;
        }

        while(nextc == ' ' || nextc == '\t' || nextc == '\n') {
            next();
        }

        return gotWhitespace;
    }

    bool isWhitespace() {
        return (nextc == ' ' || nextc == '\t' || nextc == '\n');
    }

};

struct BINARY_BLOCK;

std::set<std::string> registered_labels;
using BLOCK = std::variant<std::monostate, uint8_t, BINARY_BLOCK*>;

struct BINARY_BLOCK {
    std::vector<BLOCK> blocks;
    std::map<std::string, uint32_t> labels;
    std::map<uint32_t, std::string> references;

    // replace reference would have to be appleid to 4 adjecent uint8_t-s

    void addByte(uint8_t b) {
        blocks.push_back(b);
    }

    void addByte(uint8_t b, std::string label) {
        if(registered_labels.contains(label)) {
            printf("ERR: Label already in use: %s", label.c_str());
            std::exit(1);
            return;
        }

        registered_labels.insert(label);

        uint32_t index = blocks.size();
        blocks.push_back(b);
        labels[label] = index;
    }

    void addBinaryBlock(BINARY_BLOCK* binaryBlock) {
        blocks.push_back(binaryBlock);
    }

    void addReference(std::string label) {
        uint32_t index = blocks.size();
        blocks.push_back((uint8_t) 0);
        blocks.push_back((uint8_t) 0);
        blocks.push_back((uint8_t) 0);
        blocks.push_back((uint8_t) 0);
        references[index] = label;
    }

    void populateReferences(uint32_t absoluteOffset, uint32_t (*getAddr)(std::string)) {
        for(auto p : references) {
            uint32_t addr = getAddr(p.second);
            
            uint8_t bytes[] = {
                (uint8_t) (addr & 0xFF),
                (uint8_t) ((addr >> 8) & 0xFF),
                (uint8_t) ((addr >> 16) & 0xFF),
                (uint8_t) ((addr >> 24) & 0xFF),
            };

            blocks[p.first + 0] = bytes[0];
            blocks[p.first + 1] = bytes[1];
            blocks[p.first + 2] = bytes[2];
            blocks[p.first + 3] = bytes[3];
        }
    }






    




};






std::vector<BINARY_BLOCK> file;
Eater eater;


std::optional<std::string> parseLabel() {
    std::string label;

    if(eater.peek() < 'a' || eater.peek() > 'z') {
        return std::nullopt;
    }

    while(  (eater.peek() >= 'a' && eater.peek() <= 'z')
            || (eater.peek() >= 'A' && eater.peek() <= 'Z')) {
        label.push_back(eater.next());
    }

    if(!eater.eat(':')) {
        printf("ERR: Excepted ':' after label\n");
        std::exit(1);
    }

    printf("got label: '%s'\n", label.c_str());

    return label;
}



BINARY_BLOCK* parseArg() {
    if(eater.eat('$')) {
        std::string name;
        while(eater.peek() >= 'A' && eater.peek() <= 'Z') {
            name.push_back(eater.next());
        }

        if(!register_names.contains(name)) {
            printf("ERR: No such register %s\n", name.c_str());
            std::exit(1);
        }

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->addByte(register_names[name]);

        printf("got register: '%s'\n", name.c_str());
        
        return binaryBlock;
    }

    else if(eater.eat(':')) {
        std::string name;
        while(  (eater.peek() >= 'a' && eater.peek() <= 'z')
                || (eater.peek() >= 'A' && eater.peek() <= 'Z')) {
            name.push_back(eater.next());
        }

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->addReference(name);

        return { binaryBlock };
    }

    else {
        std::string txt;
        while(!eater.isWhitespace()) {
            const char c = eater.next();
            printf("parsing: '%c'\n", c);
            txt.push_back(c);
        }
        if(txt.length() < 3) {
            printf("ERR: Invalid literal: %s\n", txt.c_str());
            std::exit(1);
        }

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        if(txt.ends_with("u8")) {
            txt = txt.substr(0, txt.length() - 2);
            uint8_t v = lexical_cast<uint8_t>(txt);

            binaryBlock->addByte(v & 0xFF);
        }
        else if(txt.ends_with("u16")) {
            txt = txt.substr(0, txt.length() - 3);
            uint16_t v = lexical_cast<uint16_t>(txt);

            binaryBlock->addByte(v & 0xFF);
            binaryBlock->addByte((v >> 8) & 0xFF);
        }
        else if(txt.ends_with("u32")) {
            txt = txt.substr(0, txt.length() - 3);
            uint32_t v = lexical_cast<uint32_t>(txt);

            binaryBlock->addByte(v & 0xFF);
            binaryBlock->addByte((v >> 8) & 0xFF);
            binaryBlock->addByte((v >> 16) & 0xFF);
            binaryBlock->addByte((v >> 24) & 0xFF);
        }
        else {
            printf("ERR: Invalid literal\n");
            std::exit(1);
        }

        return { binaryBlock };
    }
}


BINARY_BLOCK* parseInstruction() {
    std::optional<std::string> label = parseLabel();
    eater.skipWhitespace();

    std::string instruction;

    if(eater.peek() < 'A' || eater.peek() > 'Z') {
        return 0;
    }

    while(eater.peek() >= 'A' && eater.peek() <= 'Z') {
        instruction.push_back(eater.next());
    }

    if(!instrsuction_names.contains(instruction)) {
        printf("ERR: Unknown instructions: %s\n", instruction.c_str());
        std::exit(1);
    }

    BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();

    if(label.has_value()) {
        binaryBlock->addByte(instrsuction_names[instruction], *label);
    } else {
        binaryBlock->addByte(instrsuction_names[instruction]);
    }

    printf("got instruction: '%s'\n", instruction.c_str());

    while(!eater.eat('\n')) {
        if(!eater.skipSeparator()) {
            printf("ERR: Invalid state, should've gotten whitespace\n");
            std::exit(1);
        }
        binaryBlock->addBinaryBlock(parseArg());
    }

    return binaryBlock;
}


int main(int argc, const char* argv[]) {
    if(argc < 2) {
        printf("ERR: Use syntax: assembler <asm file>\n");
        return 1;
    }

    printf("opening file: %s\n", argv[1]);
    eater.init(fopen(argv[1], "r"));

    BINARY_BLOCK binaryBlock;
    

    while(eater.peek() != EOF) {
        BINARY_BLOCK* bb = parseInstruction();
        if(!bb) break;
        binaryBlock.addBinaryBlock(bb);
    }
}



