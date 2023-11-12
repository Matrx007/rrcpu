#include <cstdlib>
#include <stdio.h>
#include <string_view>
#include <variant>
#include <vector>
#include <inttypes.h>
#include <map>
#include <string>
#include <set>
#include <optional>
#include <sstream>
#include <iostream>
#include <stdarg.h>

struct BINARY_BLOCK;
BINARY_BLOCK* parseArg();
void err(const char* fmt, ...);

std::map<std::string, uint8_t> register_names = {
    { "A", 0xAA },
    { "B", 0xBB },
    { "C", 0xCC },
    { "D", 0xDD },
    { "E", 0xEE },
    { "F", 0xFF },

    { "CA", 0xCA },
    { "CB", 0xCB },

    { "PC", 0x8c },
    { "MAR", 0x8a },
    { "MDR", 0x8d },

    { "SS", 0x60 },
    { "SP", 0x61 },
    { "SF", 0x62 },
};


std::map<std::string, uint8_t> instrsuction_names = {
    { "NOOP",    0xff },
    { "HALT",    0x00 },
    { "MVI",     0x01 },
    { "MOV",     0x02 },
    { "ADD",     0x03 },
    { "ADDI",    0x04 },
    { "SHFL",    0x05 },
    { "SHFR",    0x06 },
    { "SUB",     0x07 },
    { "SUBI",    0x08 },
    { "ROTL",   0x09 },
    { "ROTR",   0x0a },
    { "LDA",    0x20 },
    { "STA",    0x21 },
    { "JMP",    0x40 },
    { "JMPZ",   0x41 },
    { "JMPE",   0x42 },
    { "JMPI",   0x43 },
    { "JMPR",   0x44 },
    { "JMPL",   0x43 },
    { "JMPS",   0x43 },
    { "PUSH",   0x60 },
    { "POP",    0x61 },
    { "GARG",   0x62 },
    { "SARG",   0x63 },
    { "GLOC",   0x64 },
    { "SLOC",   0x65 },
    { "CALL",   0x6e },
    { "RET",    0x6f },
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
    size_t lineNr = 1;
    size_t curLineLoc = 0;
    size_t loc = 0;

    Eater() {}

    void init(FILE* file) {
        this->file = file;
        next();
    }

    char peek() {
        return nextc;
    }

    char __next() {
        char c = nextc;
        nextc = getc(file);
        loc++;
        if(nextc == '\n') curLineLoc = loc, lineNr++;
        return c;
    }

    char next() {
        const char c = __next();
        __skipComment();
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

    void __skipComment() {
        if(nextc != '#') return;
        __next();

        while(nextc != '\n' && nextc != EOF) {
            __next();
        }

        // if(nextc == '\n') __next();
    }

    bool skipWhitespace() {
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

    bool nextLine() {
        bool gotNewline = false;
        bool inComment = false;

        while(inComment || nextc == ' ' || nextc == '\t' || nextc == '\n') {
            if(nextc == '\n') inComment = false, gotNewline = true;
            if(nextc == '#') inComment = true;
            next();
        }

        return gotNewline;
    }

    bool isWhitespace() {
        return (nextc == ' ' || nextc == '\t' || nextc == '\n');
    }

};

struct BINARY_BLOCK;

std::set<std::string> registered_labels;
struct LABEL_REF { std::string label; };
struct LABEL_DEF { std::string label; };
struct STRING_REF { std::string txt; };
using BLOCK = std::variant<std::monostate, uint8_t, BINARY_BLOCK*, LABEL_REF, LABEL_DEF, STRING_REF>;

struct BINARY_BLOCK {
    std::vector<BLOCK> blocks;
    uint32_t offset;

    // replace reference would have to be appleid to 4 adjecent uint8_t-s

    void addByte(uint8_t b) {
        blocks.push_back(b);
        offset++;
    }

    void addLabel(std::string label) {
        if(registered_labels.contains(label)) {
            err("ERR: Label already in use: %s", label.c_str());
            return;
        }
        blocks.push_back(LABEL_DEF{ label });
    }

    void addByte(uint8_t b, std::string label) {
        addLabel(label);

        uint32_t index = blocks.size();
        blocks.push_back(b);

        offset++;
    }

    void addBinaryBlock(BINARY_BLOCK* binaryBlock) {
        blocks.push_back(binaryBlock);
    }

    void addReference(std::string label) {
        blocks.push_back(LABEL_REF{ label });
    }

    void addStringRef(std::string txt) {
        blocks.push_back(STRING_REF{ txt });
    }



    static uint32_t size(BLOCK block) {
        if(block.index() == 1) {
            return 1;
        } else if(block.index() == 2) {
            return (std::get<BINARY_BLOCK*>(block)->blocks.size());
        } else if(block.index() == 3) {
            return 4;
        } else if(block.index() == 4) {
            return 0;
        } else {
            err("ERR: size(): Got std::monostate\n");
        }
    }

    private:
    BINARY_BLOCK __pack() {
        return __pack(0);
    }

    BINARY_BLOCK __pack(uint32_t offset) {
        BINARY_BLOCK binaryBlock;

        for(auto var : blocks) {
            if(var.index() == 1) {
                binaryBlock.addByte(std::get<uint8_t>(var));
            } else if(var.index() == 2) {
                BINARY_BLOCK* otherBlock = std::get<BINARY_BLOCK*>(var);

                BINARY_BLOCK packedOtherBlock = otherBlock->__pack(offset);

                for(auto otherBlockVar : packedOtherBlock.blocks) {
                    if(otherBlockVar.index() == 2) {
                        err("ERR: pack(): Found BINARY_BLOCK, even after calling pack() recursively\n");
                    }

                    if(otherBlockVar.index() == 1) binaryBlock.blocks.push_back(std::get<uint8_t>(otherBlockVar));
                    else if(otherBlockVar.index() == 3) binaryBlock.blocks.push_back(std::get<LABEL_REF>(otherBlockVar));
                    else if(otherBlockVar.index() == 4) binaryBlock.blocks.push_back(std::get<LABEL_DEF>(otherBlockVar));
                    else if(otherBlockVar.index() == 5) binaryBlock.blocks.push_back(std::get<STRING_REF>(otherBlockVar));
                    else err("ERR: pack(): got monostate\n");
                }
            } else if(var.index() == 3) {
                binaryBlock.blocks.push_back(std::get<LABEL_REF>(var));
            } else if(var.index() == 4) {
                binaryBlock.blocks.push_back(std::get<LABEL_DEF>(var));
            } else if(var.index() == 5) {
                binaryBlock.blocks.push_back(std::get<STRING_REF>(var));
            } else {
                err("ERR: pack(): got monostate\n");
            }
        }

        return binaryBlock;
    }
    
    public:
    std::vector<uint8_t> compile() {
        std::vector<uint8_t> res;
        BINARY_BLOCK binaryBlock = __pack();
        std::map<std::string, uint32_t> labelDefs;
        std::map<uint32_t, std::string> labelRefs;
        std::map<std::string, uint32_t> stringDefs;
        std::map<uint32_t, std::string> stringRefs;

        uint32_t offset = 0;
        uint8_t skipCount = 0;
        for(auto var : binaryBlock.blocks) {
            if(skipCount > 0) { skipCount--; continue; }

            switch (var.index()) {
            
            case 0:
            err("ERR: compile(): Got std::monostate\n");
            break;

            case 1:
            res.push_back(std::get<uint8_t>(var));
            offset++;
            break;
            
            case 2:
            err("ERR: compile(): Got BINARY_BLOCK\n");
            break;

            case 3:
            labelRefs[offset] = std::get<LABEL_REF>(var).label;
            res.push_back(0);
            res.push_back(0);
            res.push_back(0);
            res.push_back(0);
            offset += 4;
            break;

            case 4:
            labelDefs[std::get<LABEL_DEF>(var).label] = offset;
            break;

            case 5:
            stringRefs[offset] = std::get<STRING_REF>(var).txt;
            res.push_back(0);
            res.push_back(0);
            res.push_back(0);
            res.push_back(0);
            offset += 4;
            break;

            }
        }

        res.push_back(0), offset++;

        for(auto pair : stringRefs) {
            if(stringDefs.contains(pair.second)) continue;
            stringDefs[pair.second] = offset;
            res.insert(res.end(), pair.second.begin(), pair.second.end());
            res.push_back(0);
            offset += pair.second.size() + 1;
        }

        for(auto pair : labelRefs) {
            uint32_t addr = labelDefs[pair.second];

            res[pair.first + 0] = (addr >>  0) & 0xFF;
            res[pair.first + 1] = (addr >>  8) & 0xFF;
            res[pair.first + 2] = (addr >> 16) & 0xFF;
            res[pair.first + 3] = (addr >> 24) & 0xFF;
        }

        for(auto pair : stringRefs) {
            uint32_t addr = stringDefs[pair.second];

            res[pair.first + 0] = (addr >>  0) & 0xFF;
            res[pair.first + 1] = (addr >>  8) & 0xFF;
            res[pair.first + 2] = (addr >> 16) & 0xFF;
            res[pair.first + 3] = (addr >> 24) & 0xFF;
        }

        return res;
    }



};





typedef BINARY_BLOCK* (*CommandFn)(void);
std::vector<BINARY_BLOCK> file;
Eater eater;
std::map<std::string, CommandFn> commands = {
    { "empty", [](void) -> BINARY_BLOCK* {
        std::string nr;
        eater.skipWhitespace();
        while(eater.peek() >= '0' && eater.peek() <= '9') {
            nr.push_back(eater.next());
        }
        if(nr.empty()) err("ERR: .empty: Expected byte count\n");

        uint32_t count = atoi(nr.c_str());

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->blocks.reserve(count);
        for(uint32_t i = 0; i < count; i++) {
            binaryBlock->addByte(0);
        }
        return binaryBlock;
    } },

    { "data", [](void) -> BINARY_BLOCK* {
        eater.skipWhitespace();

        BINARY_BLOCK* block = new BINARY_BLOCK();
        BINARY_BLOCK* parsed = 0;
        for(;;) {
            parsed = parseArg();

            if(!parsed) err("ERR: .data: Expected at least one data block\n");

            block->blocks.push_back(parsed);

            if(eater.nextLine()) break;
        }

        if(block->blocks.size() == 0) {
            err("ERR: .data: Expected at least one data block\n");
        }

        return block;
    } }
};

std::optional<std::string> parseLabel() {
    std::string label;

    if(eater.peek() < 'a' || eater.peek() > 'z') {
        return std::nullopt;
    }

    while(  (eater.peek() >= 'a' && eater.peek() <= 'z')
            || (eater.peek() >= 'A' && eater.peek() <= 'Z')
            || (eater.peek() >= '0' && eater.peek() <= '9')
            || (eater.peek() == '_')) {
        label.push_back(eater.next());
    }

    if(!eater.eat(':')) {
        err("ERR: Excepted ':' after label\n");
    }

    return label;
}



BINARY_BLOCK* parseArg() {
    if(eater.eat('$')) {
        std::string name;
        while(eater.peek() >= 'A' && eater.peek() <= 'Z') {
            name.push_back(eater.next());
        }

        if(!register_names.contains(name)) {
            err("ERR: No such register %s\n", name.c_str());
        }

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->addByte(register_names[name]);
        
        return binaryBlock;
    }

    else if(eater.eat(':')) {
        std::string name;
        if(eater.peek() < 'a' || eater.peek() > 'z') err("ERR: parseArg(): Invalid first character in label\n");

        while(  (eater.peek() >= 'a' && eater.peek() <= 'z')
                || (eater.peek() >= 'A' && eater.peek() <= 'Z')
                || (eater.peek() >= '0' && eater.peek() <= '9')
                || (eater.peek() == '_')) {
            name.push_back(eater.next());
        }

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->addReference(name);

        return binaryBlock;
    }

    else if(eater.eat('\'')) {
        char c = eater.next();
        if(c == '\'') err("ERR: parseArg(): Expected character after '\n");
        if(c == '\\') {
            switch(c = eater.next()) {
                case '\'':
                c = '\'';
                break;

                case '\\':
                c = '\\';
                break;

                case 'n':
                c = '\n';
                break;

                case '0':
                c = '\0';
                break;

                default:
                err("ERR: parseArg(): Uknown escape sequence: \\%c (0x%x)", c, c);
            }
        }

        if(!eater.eat('\'')) err("ERR: parseArg(): Missing closing quote\n");

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->addByte(c);

        return binaryBlock;
    }

    else if(eater.eat('"')) {
        std::string txt;
        bool parsingEscape = false;
        for(;;) {
            const char c = eater.next();
            if(c == '\\') {
                parsingEscape = true;
                continue;
            }

            if(parsingEscape) {
                switch(c) {
                
                case 'n':
                txt.push_back('\n');
                break;
                
                case '"':
                txt.push_back('"');
                break;
                
                case '0':
                err("ERR: parseArg(): \\0 is not allowed in referenced string\n");
                break;

                default:
                err("ERR: parseArg(): Unknown escape: '\\%c' (0x%x)\n", c, c);

                }

                parsingEscape = false;
                continue;
            }

            if(c == '"') break;

            txt.push_back(c);
        }

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();
        binaryBlock->addStringRef(txt);

        return binaryBlock;
    }

    else if(eater.eat('b')) {
        if(!eater.eat('"')) err("ERR: parseArg(): Expected b\", but \" was missing\n");
        

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();

        bool parsingEscape = false;
        for(;;) {
            const char c = eater.next();
            if(c == '\\') {
                parsingEscape = true;
                continue;
            }

            if(parsingEscape) {
                switch(c) {
                
                case 'n':
                binaryBlock->addByte('\n');
                break;
                
                case '"':
                binaryBlock->addByte('"');
                break;
                
                case '0':
                binaryBlock->addByte(0);
                break;

                default:
                err("ERR: parseArg(): Unknown escape: '\\%c' (0x%x)\n", c, c);

                }

                parsingEscape = false;
                continue;
            }

            if(c == '"') break;

            binaryBlock->addByte(c);
        };

        return binaryBlock;
    }

    else {
        std::string txt;
        while(!eater.isWhitespace()) {
            const char c = eater.next();
            txt.push_back(c);
        }
        if(txt.length() < 3) {
            err("ERR: Invalid literal: %s\n", txt.c_str());
        }

        uint8_t base = 10;
        if(txt.starts_with("0x")) base = 16, txt = txt.substr(2);
        else if(txt.starts_with("0b")) base = 2, txt = txt.substr(2);

        BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();

        uint32_t mask = 0;
        if(txt.ends_with("u8")) mask = 0xFF, txt = txt.substr(0, txt.length() - 2);
        else if(txt.ends_with("u16")) mask = 0xFFFF, txt = txt.substr(0, txt.length() - 3);
        else if(txt.ends_with("u32")) mask = 0xFFFFFFFF, txt = txt.substr(0, txt.length() - 3);
        else err("ERR: Invalid literal\n");

        uint32_t v = strtol(txt.c_str(), NULL, base);

        if(mask == 0xFF) {
            binaryBlock->addByte(v & 0xFF);
        } else if(mask == 0xFFFF) {
            binaryBlock->addByte(v & 0xFF);
            binaryBlock->addByte((v >> 8) & 0xFF);
        } else if(mask == 0xFFFFFFFF) {
            binaryBlock->addByte(v & 0xFF);
            binaryBlock->addByte((v >> 8) & 0xFF);
            binaryBlock->addByte((v >> 16) & 0xFF);
            binaryBlock->addByte((v >> 24) & 0xFF);
        }

        return binaryBlock;
    }
}


BINARY_BLOCK* parseInstruction() {
    eater.skipWhitespace();

    std::string instruction;

    if(eater.peek() < 'A' || eater.peek() > 'Z') {
        return 0;
    }

    while(eater.peek() >= 'A' && eater.peek() <= 'Z') {
        instruction.push_back(eater.next());
    }

    if(!instrsuction_names.contains(instruction)) {
        err("ERR: Unknown instructions: %s\n", instruction.c_str());
    }

    BINARY_BLOCK* binaryBlock = new BINARY_BLOCK();

    binaryBlock->addByte(instrsuction_names[instruction]);

    while(!eater.nextLine()) {
        binaryBlock->addBinaryBlock(parseArg());
    }

    return binaryBlock;
}


BINARY_BLOCK* parseCommand() {
    eater.skipWhitespace();
    if(!eater.eat('.')) return 0;

    std::string name;

    while((eater.peek() >= 'a' && eater.peek() <= 'z')
            || (eater.peek() >= 'A' && eater.peek() <= 'Z')) {
        name.push_back(eater.next());
    }

    if(!commands.contains(name)) {
        err("ERR: No such command %s\n", name.c_str());
    }

    return commands[name]();
}


void err(const char* fmt, ...) {
    putchar('\n');

    va_list argptr;
    va_start(argptr, fmt);

    vprintf(fmt, argptr);

    va_end(argptr);

    printf("%5ld: ", eater.lineNr);

    fseek(eater.file, eater.curLineLoc, SEEK_SET);
    char c;
    while((c = fgetc(eater.file)) != '\n' && c != EOF) {
        putchar(c);
    }
    putchar('\n');

    for(int i = 0; i < 7 + (eater.loc - eater.curLineLoc) - 1; i++) {
        putchar(' ');
    }
    putchar('^');

    putchar('\n');
    putchar('\n');

    std::exit(1);
}

int main(int argc, const char* argv[]) {
    if(argc < 3) {
        err("ERR: Use syntax: assembler <asm file> <out file>\n");
        return 1;
    }

    eater.init(fopen(argv[1], "r"));

    FILE* out = fopen(argv[2], "wb");

    BINARY_BLOCK binaryBlock;
    

    while(eater.peek() != EOF) {
        eater.skipWhitespace(); 
        {
            std::optional<std::string> label = parseLabel();
            if(label.has_value()) {
                binaryBlock.blocks.push_back(LABEL_DEF{ *label });
                eater.nextLine();
                continue;
            }
        }

        BINARY_BLOCK* bb = parseInstruction();
        if(!bb) bb = parseCommand();
        if(!bb) err("ERR: Uknown statement\n");
        binaryBlock.addBinaryBlock(bb);
        eater.nextLine();
    }

    std::vector<uint8_t> binary = binaryBlock.compile();

    fwrite(binary.data(), binary.size(), 1, out);
    fclose(out);
}



