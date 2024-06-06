#include <iostream>
#include "lexer.h"
#include <fstream>
#include "parser.h"
#include "codegen.h"

int main(int argc, char* argv[])
{
    std::string source;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        
        std::string line;

        while (getline(file, line))
            source += line + '\n';
        source.pop_back();
        file.close();
    }
    else {
        std::cout << "> ";
        std::getline(std::cin, source);
    }
    ErrorHandler error_handler(source);
    Lexer lexer(source, &error_handler);
    lexer.analyze();
    if (error_handler.has_error()) {
        error_handler.output_errors();
        
    }
    else {
        for (Token& tok : lexer.out) {
            std::cout << tok.type << " " << tok.value << "\n";
        }
        Parser parser(lexer.out, &error_handler);
        std::shared_ptr<AST> ast = parser.parse();
        
        if (error_handler.has_error()) {
            error_handler.output_errors();
        }
        else {
            parser.print_ast();
            CodeGenerator code_gen(ast, &error_handler);
            code_gen.generate_asm();
            if (error_handler.has_error()) {
                error_handler.output_errors();
            }
            else {
                std::cout << code_gen.assembly_out;

                if (argc > 1) {
                    std::ofstream file(std::string(argv[1]) + ".s");

                    file << code_gen.assembly_out;
                    file.close();
                }
            }
        }

        
    }

    
}