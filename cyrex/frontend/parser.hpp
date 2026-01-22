#pragma once
#include "token.hpp"
#include "ast.hpp"
#include <span>

class Parser {
private:
	enum class Context {
		TopLevel,
		ParameterList,
		Statement,
		Expression
	};
public:
	struct Error {
		constexpr explicit Error(const std::string& message);
		std::string message;
	};

	AST::Ptr parse(const std::vector<Token>& tokens);
	bool has_errors() const;
	const std::vector<Error>& get_errors() const;

private:
	bool is_at_end() const;
	const Token& current_token() const;
	bool check(const Token::Type type) const;
	bool check_binary() const;
	bool expect(const Token::Type type, const std::string& error_message);
	bool expect_data_type(const std::string& error_message);
	Token next();

	void push_error(const std::string& error_message);

	AST::Ptr parse_top();
	AST::Ptr parse_stmt();
	AST::Ptr parse_expr();

	AST::Ptr parse_tuple(int size_limit = -1);
	AST::Ptr parse_if(Context context);
	AST::Ptr parse_while(Context context);
	AST::Ptr parse_return();


	AST::Ptr parse_function();
	AST::Ptr parse_block();
	AST::Ptr parse_parameters();
	AST::Ptr parse_variable(Context context);

	AST::Ptr parse_identifier();
	AST::Ptr parse_number();
	AST::Ptr parse_binary(AST::Ptr&& left);

	AST::Type parse_type();

private:
	std::vector<Token> tokens;
	size_t index{};
	std::vector<Error> errors;
};