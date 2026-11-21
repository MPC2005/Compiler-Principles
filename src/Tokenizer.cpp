#include "Lexer.h"

#include <iostream>
#include <iterator>

static void writeTokens(const std::vector<toy::Token>& tokens) {
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        std::cout << i << ':' << tokens[i].type << ":\""
                  << tokens[i].text << "\"\n";
    }
}

int main() {
    const std::string source((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    toy::Lexer lexer(source);
    std::vector<toy::Token> tokens;
    std::vector<toy::LexError> errors;
    lexer.scan(tokens, errors);
    if (!errors.empty()) {
        std::cout << "reject\n";
        for (const toy::LexError& error : errors) std::cout << error.line << ' ' << error.message << '\n';
        std::cout << '\n';
    }
    writeTokens(tokens);
    return errors.empty() ? 0 : 1;
}
