#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "token.h"

class Error {
public:
	Error(std::string message, Token token) : message(message), token(token) {}			// Error struct holds error info like error message, line, start and end index of error

	std::string message;
	Token token;
};

class ErrorHandler {
public:
	ErrorHandler(const std::string& input) : input(input) {}							// Error handler is a class that holds a list of errors
	const std::string& input;															// Error output and input is handled with ease through their use
	std::vector<Error> errors;
	
	void report_error(const std::string message, Token token) {							// Add an error to errors vector
		errors.push_back(Error( message, token) );
	}
	bool has_error() {																	// Check if has errors
		return errors.size() > 0;
	}
	void output_errors() {																// Print all errors
		for (Error& error : errors) {
			std::cout << "Error at line " << error.token.line << ": " << error.message << '\n';
		}
	}
};