#include "pch.h"
#include "lexer.h"
#include <string>
#include <iostream>

Token Lexer::lex() {
    next();
    
    while (current_char == ' ' || current_char == '\n' || current_char == '\t' || current_char == '/') {    // Skip whitespace
        if (current_char == '\n')                                                                           // Change line on newline character
            line++;
        else if (current_char == '/') {                                                                     // Skip comments
            if (match('/')) {
                while (current_char != '\n' && index < source.size())
                    next();
            }
            else if (index < source.size() && match('*')) {
                next();
                next();
                while (index < source.size()) {
                    if (current_char == '\n') {
                        line++;
                        next();
                    }
                    else if (current_char == '*' && match('/')) {
                        next();
                        next();
                        break;
                    }
                    else {
                        next();
                    }
                }
            }
            else
                break;
        }
        next();
    }
    if (is_digit(current_char)) return (number());                        // Make number token
    if (is_alpha(current_char)) return (identifier());                    // Make identifier/keyword token

    int old_index = index;                                                      // Keep track of starting index
    switch (current_char) {                                                     // Make single/double character tokens or string token
    case '(': return (Token(TOKEN_L_PAR, "", line, old_index, index));
    case ')': return (Token(TOKEN_R_PAR, "", line, old_index, index));
    case '{': return (Token(TOKEN_L_BRACE, "", line, old_index, index));
    case '}': return (Token(TOKEN_R_BRACE, "", line, old_index, index));
    case '[': return (Token(TOKEN_L_BRACK, "", line, old_index, index));
    case ']': return (Token(TOKEN_R_BRACK, "", line, old_index, index));
    case ';': return (Token(TOKEN_SEMICOLON, "", line, old_index, index));
    case ',': return (Token(TOKEN_COMMA, "", line, old_index, index));
    case '.': return (Token(TOKEN_DOT, "", line, old_index, index));
    case '^': return (Token(TOKEN_CAP, "", line, old_index, index));
    case '&': return (Token(TOKEN_AMPERSAND, "", line, old_index, index));
    case '~': return (Token(TOKEN_TILDE, "", line, old_index, index));
    case '%': return (Token(
        match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT, "", line, old_index, index));
    case '-': return (Token(
        match('=') ? TOKEN_MINUS_EQUAL : match('>') ? TOKEN_ARROW : match('-') ? TOKEN_MINUS_MINUS : TOKEN_MINUS, "", line, old_index, index));
    case '+': return (Token(
        match('=') ? TOKEN_PLUS_EQUAL : match('+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS, "", line, old_index, index));
    case '/': return (Token(
        match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH, "", line, old_index, index));
    case '*': return (Token(
        match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR, "", line, old_index, index));
    case '!':
        return (Token(
            match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG, "", line, old_index, index));
    case '=':
        return (Token(
            match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL, "", line, old_index, index));
    case '<':
        return (Token(
            match('=') ? TOKEN_LESS_EQUAL : match('<') ? TOKEN_L_SHIFT : TOKEN_LESS, "", line, old_index, index));
    case '>':
        return (Token(
            match('=') ? TOKEN_GREATER_EQUAL : match('>') ? TOKEN_R_SHIFT : TOKEN_GREATER, "", line, old_index, index));
    case '"':
        return (string());
    case '\0':
        return Token(TOKEN_EOF, "", line, index, index);
    }
    error_handler->report_error(std::string("Unexpected character '") + current_char + std::string("'"), Token(TOKEN_ERROR, "", line, old_index, index));
    return Token(TOKEN_ERROR, "", line, old_index, index);
    
}

std::vector<Token> Lexer::analyze() {
    do {  
        out.push_back(lex());
    } while (current_char != '\0');
    return out;
}

Token Lexer::string() {
    std::string string;
    bool interpolated = false;
    int old_index = index;
    next();
    while (current_char != '"' && current_char != '\0') {               // Make the string body
        if (current_char == '\n')
            line++;
        string += current_char;
        next();
    }

    if (current_char == '\0') {                                        // If the lexer reaches the end without closing the string return an error
        error_handler->report_error("Unterminated string", Token(TOKEN_ERROR, "", line, old_index, index));
        return Token(TOKEN_ERROR, "", line, old_index, index);
    }
    return Token(TOKEN_STR, string, line, old_index, index);
}

Token Lexer::number() {
    int old_index = index;
    std::string num_string;
    bool is_float = false;
    bool has_error = false;
    int dot_index = 0;
    
    while (is_digit(current_char) || current_char == '.' || current_char == '_') {          // Make number string
        if (current_char == '.') {
            if (is_float) {
                has_error = true;                                                           // If there already was a dot, report error
                dot_index = index;
            }
            else
                is_float = true;                                                            // Else set the type as float
            num_string += current_char;
            next();
            
        }
        else if (current_char == '_')                                                       // Allow underscores in numbers for readability
            next();
        else {
            
            num_string += current_char;
            next();
        }
    }
    back();

    if (has_error) {                                                                        // Error double dot
        error_handler->report_error("Unexpected '.'", Token(TOKEN_ERROR, "", line, dot_index, dot_index));
        return Token(TOKEN_ERROR, "", line, dot_index, dot_index);
    }
    if(is_float)
        return Token(TOKEN_FLOAT, num_string, line, old_index, index);
    return Token(TOKEN_INT, num_string, line, old_index, index);
}

Token Lexer::identifier() {
    int old_index = index;
    std::string string;

    while (is_alpha(current_char) || is_digit(current_char)) {                              // Make identifier body
        string += current_char;
        next();
    }
    back();

    if (std::find(keywords.begin(), keywords.end(), string) != keywords.end())              // Check if name is identifier or keyword
        return Token(TOKEN_KEYWORD, string, line, old_index, index);
    else if (string == "and")
        return Token(TOKEN_AND, string, line, old_index, index);
    else if (string == "or")
        return Token(TOKEN_OR, string, line, old_index, index);
    return Token(TOKEN_ID, string, line, old_index, index);                    
}

bool Lexer::match(char expected) {                                                          // If next character matches the expected one,
    if (index + 1 >= source.size() || source[index + 1] != expected) return false;          // advance and return true, else return false
    next();
    return true;
}

void Lexer::back() {
    index--;
    current_char = source[index];
}

void Lexer::next() {
    index++;
    current_char = index < source.size() ? source[index] : '\0';
}

bool Lexer::is_alpha(char character) {
    return (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') || character == '_';
}

bool Lexer::is_digit(char character) {
    return character >= '0' && character <= '9';
}