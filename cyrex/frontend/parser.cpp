#include "parser.hpp"
#include <optional>
#include <format>

template<typename T>
static AST::Ptr make_ast(T&& val, std::optional<AST::Symbol> sym = std::nullopt) {
	return AST::Ptr(new AST{ .data = std::move(val), .sym = sym });
}


constexpr Parser::Error::Error(const std::string& message) : message(message) {
}

bool Parser::is_at_end() const {
	return index >= tokens.size();
}

const Token& Parser::current_token() const {
	return tokens[index];
}

bool Parser::check(const Token::Type type) const {
	return !is_at_end() && current_token().type == type;
}

bool Parser::check_binary() const {
	const auto current = current_token().type;
	switch (current) {
		case Token::Type::Equal:
		case Token::Type::NotEqual:
		case Token::Type::Lesser:
		case Token::Type::LesserOrEqual:
		case Token::Type::Greater:
		case Token::Type::GreaterOrEqual:
		case Token::Type::And:
		case Token::Type::Or:
		case Token::Type::Xor:
		case Token::Type::Plus:
		case Token::Type::Minus:
		return true;
	}
	return false;
}

bool Parser::expect(const Token::Type type, const std::string& error_message) {
	bool same = check(type);
	if (!same) push_error(std::string(error_message));
	return same;
}

bool Parser::expect_data_type(const std::string& error_message) {
	const Token& current = current_token();
	switch (current.type) {
		case Token::Type::Void:
		case Token::Type::Char:
		case Token::Type::Short:
		case Token::Type::Int:
		case Token::Type::Long:
		// may be user defined
		case Token::Type::Identifier:
		return true;
	}
	push_error(error_message);
	return false;
}

Token Parser::next() {
	return tokens[index++];
}

void Parser::push_error(const std::string& error_message) {
	errors.emplace_back(error_message);
}

AST::Ptr Parser::parse(const std::vector<Token>& tokens) {
	this->tokens = tokens;
	this->errors.clear();
	this->errors.shrink_to_fit();
	this->index = 0;
	auto root = AST::Root{};

	while (!is_at_end()) {
		auto ast = parse_top();
		if (!ast) { break; }
		root.functions.emplace_back(std::move(ast));
	}

	return make_ast(root);
}

bool Parser::has_errors() const {
	return !errors.empty();
}

const std::vector<Parser::Error>& Parser::get_errors() const {
	return errors;
}

AST::Ptr Parser::parse_top() {
	if (check(Token::Type::Function)) {
		next();
		return parse_function();
	}
	push_error("expected 'function'");
	return nullptr;
}

AST::Ptr Parser::parse_stmt() {
	if (check(Token::Type::If)) {
		next();
		return parse_if(Context::Statement);
	}
	if (check(Token::Type::While)) {
		next();
		return parse_while(Context::Statement);
	}
	if (check(Token::Type::Return)) {
		next();
		return parse_return();
	}
	if (check(Token::Type::Var)) {
		return parse_variable(Context::Statement);
	}
	if (check(Token::Type::Const)) {
		return parse_variable(Context::Statement);
	}
	if (check(Token::Type::Identifier)) {
		return parse_identifier();
	}
	if (check(Token::Type::LeftSqBracket)) {
		next();
		return parse_tuple();
	}

	push_error(std::format("unexpected {} in statement", current_token().text));
	return nullptr;
}

AST::Ptr Parser::parse_expr() {
	if (check(Token::Type::Number)) {
		return parse_number();
	}

	if (check(Token::Type::String)) {
		return make_ast(AST::LiteralExpr{ .type =
			AST::Type{
				.name = "byte",
				.qualifiers = {
					AST::Type::Qualifier{
						.kind = AST::Type::Qualifier::Kind::Pointer
					}
				}
			}, .value = next().text });

	}

	if (check(Token::Type::Identifier)) {
		return parse_identifier();
	}

	if (check(Token::Type::While)) {
		next();
		return parse_while(Context::Expression);
	}

	if (check(Token::Type::If)) {
		next();
		return parse_if(Context::Expression);
	}

	if (check(Token::Type::LeftSqBracket)) {
		next();
		return parse_tuple();
	}

	return nullptr;
}

AST::Ptr Parser::parse_tuple(const int size_limit) {
	std::vector<AST::Ptr> exprs;

	while (!check(Token::Type::RightSqBracket)) {
		auto expr = parse_expr();

		if (!expr) {
			push_error("expected expression in tuple");
			return nullptr;
		}
		exprs.push_back(std::move(expr));
		if (check(Token::Type::RightSqBracket)) break;

		if (!expect(Token::Type::Comma, "expected comma after expression")) {
			return nullptr;
		}
		next();
	}

	if (!expect(Token::Type::RightSqBracket, "expected ] to close tuple")) {
		return nullptr;
	}
	next();

	if (exprs.empty()) {
		push_error("tuple cannot be empty");
		return nullptr;
	}

	if (size_limit != -1 && exprs.size() != size_limit) {
		push_error(std::format("tuple must have exactly {} terms", size_limit));
		return nullptr;
	}

	size_t nexprs = exprs.size();

	auto tuple = make_ast(AST::TupleExpr{ .exprs = std::move(exprs) });

	// [a,b] = [b,a]
	if (check(Token::Type::Assign)) {
		next();
		if (!expect(Token::Type::LeftSqBracket, "expected [ for tuple")) {
			return nullptr;
		}
		next();

		auto rhs = parse_tuple(nexprs);

		if (rhs == nullptr) {
			push_error("expected tuple for tuple assignment");
			return nullptr;
		}

		return make_ast(AST::TupleAssignExpr{
			.tup_left = std::move(tuple),
			.tup_right = std::move(rhs) });
	}

	if (tuple == nullptr) {

	}

	return tuple;

}

AST::Ptr Parser::parse_if(Context context) {
	auto condition = parse_expr();
	if (!condition) return nullptr;

	// it is a block-if
	if (check(Token::Type::LeftBrace)) {
		next();
		AST::Ptr then_block = parse_block();
		AST::Ptr else_block = nullptr;
		if (check(Token::Type::Else)) {
			next();
			if (check(Token::Type::LeftBrace)) {
				next();
				else_block = parse_block();
			} else {
				else_block = parse_stmt();
			}
			if (!else_block) return nullptr;
		}
		return make_ast(AST::IfStmt{
			.condition = std::move(condition),
			.then_stmt = std::move(then_block),
			.else_stmt = std::move(else_block) });
	}

	// it is a statement if
	if (check(Token::Type::Do)) {
		next();

		AST::Ptr then_stmt = parse_stmt();
		AST::Ptr else_stmt = nullptr;

		if (check(Token::Type::Else)) {
			next();
			else_stmt = parse_stmt();
		}

		return make_ast(AST::IfStmt{
			.condition = std::move(condition),
			.then_stmt = std::move(then_stmt),
			.else_stmt = std::move(else_stmt) });
	}

	if (!expect(Token::Type::Then, "expected 'then' for if-expression")) {
		return nullptr;
	}
	next();

	// it is an if-expression
	auto then_expr = parse_expr();
	if (!then_expr) return nullptr;

	if (!expect(Token::Type::Else, "expected else after if-expression")) return nullptr;
	next();

	auto else_expr = parse_expr();
	if (!else_expr) return nullptr;

	return make_ast(AST::IfExpr{
		.condition = std::move(condition),
		.then_expr = std::move(then_expr),
		.else_expr = std::move(else_expr) });
}

AST::Ptr Parser::parse_while(Context context) {
	auto condition = parse_expr();
	if (!condition) return nullptr;

	if (check(Token::Type::LeftBrace)) {
		next();
		auto block = parse_block();
		if (check(Token::Type::Then)) {
			next();
			auto expr = parse_expr();
			if (!expr) return nullptr;
			// while condition {...} then expr
			return make_ast(AST::WhileExpr{
				.condition = std::move(condition),
				.block = std::move(block),
				.returns = std::move(expr)
				});
		}

		// standard C style while loop
		return make_ast(AST::WhileStmt{
			.condition = std::move(condition),
			.block = std::move(block),
			});
	}

	if (check(Token::Type::Do)) {
		next();
		if (check(Token::Type::LeftBrace)) {
			push_error("unexpected { after while statement");
			return nullptr;
		}
		auto stmt = parse_stmt();
		if (!stmt) return nullptr;

		if (check(Token::Type::Then)) {
			next();
			// const y : int = while x < 10 x = x + 1 then x * 2
			auto expr = parse_expr();
			if (!expr) return nullptr;
			return make_ast(AST::WhileExpr{
				.condition = std::move(condition),
				.block = std::move(stmt),
				.returns = std::move(expr)
				});
		}

		// while x < 10 do ...
		return make_ast(AST::WhileStmt{
			.condition = std::move(condition),
			.block = std::move(stmt),
			});
	}

	push_error(std::format("unexpected {} after while condition", current_token().text));
	return nullptr;
}

AST::Ptr Parser::parse_return() {
	AST::ReturnStmt ret{};
	ret.expr = parse_expr();
	return make_ast(std::move(ret));
}

AST::Ptr Parser::parse_function() {
	AST::Function function{};

	// read function name
	if (!expect(Token::Type::Identifier, "expected name of function")) {
		return nullptr;
	}
	function.name = next().text;
	// read parameters
	if (!expect(Token::Type::LeftParen, std::format("expected ( after function {}'s name", function.name))) {
		return nullptr;
	}
	next();
	function.parameter_list = parse_parameters();
	if (!function.parameter_list) {
		return nullptr;
	}
	// read return type
	if (!expect(Token::Type::Colon, std::format("expected : after function {}'s parameters", function.name))) {
		return nullptr;
	}
	next();
	if (!expect_data_type(std::format("expected return type for function {} after : ", function.name))) {
		return nullptr;
	}
	function.return_type = parse_type();
	// read body
	if (!expect(Token::Type::LeftBrace, std::format("expected {{ to begin function {}'s block", function.name))) {
		return nullptr;
	}
	next();
	function.block = parse_block();
	if (!function.block) {
		push_error("error parsing block");
		return nullptr;
	}
	return make_ast(std::move(function));
}

AST::Ptr Parser::parse_block() {
	AST::BlockStmt block{};
	while (!is_at_end()) {
		// found closing brace
		if (check(Token::Type::RightBrace)) {
			next();
			break;
		}
		auto statement = parse_stmt();
		if (!statement) {
			push_error("expected statement in function body");
			return nullptr;
		}
		block.statements.emplace_back(std::move(statement));
	}

	return make_ast(std::move(block));
}

AST::Ptr Parser::parse_parameters() {
	AST::ParameterList params{};
	while (!is_at_end()) {
		if (check(Token::Type::RightParen)) {
			next();
			break;
		}
		auto variable = parse_variable(Context::ParameterList);
		if (!variable) {
			return nullptr;
		}
		params.items.emplace_back(std::move(variable));

		if (check(Token::Type::Comma)) {
			next();
			break;
		}
		if (check(Token::Type::RightParen)) {
			next();
			break;
		}
	}

	return make_ast(std::move(params));
}

AST::Ptr Parser::parse_variable(Context context) {
	AST::VariableStmt variable{};
	bool is_const = false;

	// read constness
	switch (next().type) {
		case Token::Type::Var:
		is_const = false;
		break;
		case Token::Type::Const:
		is_const = true;
		break;
		default:
		push_error("expected either const or var");
		return nullptr;
	}
	variable.is_const = is_const;

	// read name
	if (!expect(Token::Type::Identifier, "expected variable name")) {
		return nullptr;
	}
	const std::string name = next().text;
	variable.name = name;

	// read type
	if (!expect(Token::Type::Colon, "expected : after variable name")) {
		return nullptr;
	}
	next();
	if (!expect_data_type("expected variable type after :")) {
		return nullptr;
	}
	variable.type = parse_type();
	// enforce const has initializer
	if (context != Context::ParameterList && variable.is_const && !check(Token::Type::Assign)) {
		push_error(std::format("{} is const so it must be initialized", variable.name));
		return nullptr;
	}
	// read initializer expression
	if (check(Token::Type::Assign)) {
		next();
		auto expr = parse_expr();
		if (!expr) {
			push_error("expected expression for variable initializer");
			return nullptr;
		}
		variable.initializer = std::move(expr);
	}

	return make_ast(std::move(variable), AST::Symbol{ .name = name, .is_assignable = !is_const });
}

AST::Ptr Parser::parse_identifier() {
	auto name = next().text;
	auto ident = make_ast(AST::IdentifierExpr{ .name = name }, AST::Symbol{ .name = name });

	if (check_binary()) {
		return parse_binary(std::move(ident));
	}

	if (check(Token::Type::Assign)) {
		next();
		auto expr = parse_expr();
		if (!expr) return nullptr;
		return make_ast(AST::AssignExpr{
			.left = std::move(ident),
			.expr = std::move(expr) });
	}

	return ident;
}

AST::Ptr Parser::parse_number() {
	auto num = make_ast(AST::LiteralExpr{ .type =
		AST::Type {
			.name = "int",
			.qualifiers = {}
		},
		.value = next().text });

	if (check_binary()) {
		return parse_binary(std::move(num));
	}

	return num;
}

AST::Ptr Parser::parse_binary(AST::Ptr&& left) {
	const AST::BinaryExpr::Kind kind = [&]() {
		switch (next().type) {
			case Token::Type::Lesser: return AST::BinaryExpr::Kind::CmpLesser;
			case Token::Type::LesserOrEqual: return AST::BinaryExpr::Kind::CmpLesserOrEqual;
			case Token::Type::Greater: return AST::BinaryExpr::Kind::CmpGreater;
			case Token::Type::GreaterOrEqual: return AST::BinaryExpr::Kind::CmpGreaterOrEqual;
			case Token::Type::Equal: return AST::BinaryExpr::Kind::CmpEqual;
			case Token::Type::NotEqual: return AST::BinaryExpr::Kind::CmpNotEqual;
			case Token::Type::And: return AST::BinaryExpr::Kind::CmpAnd;
			case Token::Type::Or: return AST::BinaryExpr::Kind::CmpOr;
			case Token::Type::Xor: return AST::BinaryExpr::Kind::CmpXor;
			case Token::Type::Plus: return AST::BinaryExpr::Kind::Add;
			case Token::Type::Minus: return AST::BinaryExpr::Kind::Sub;
		}
	}();

	auto expr = parse_expr();
	if (!expr) {
		push_error("expected expression in right hand side of binary expression");
		return nullptr;
	}

	return make_ast(AST::BinaryExpr{
		.left = std::move(left),
		.right = std::move(expr),
		.kind = kind });
}

AST::Type Parser::parse_type() {
	AST::Type type{};
	type.name = next().text;

	while (!is_at_end()) {
		if (check(Token::Type::Star)) {
			// pointer
			type.qualifiers.push_back(
				AST::Type::Qualifier{
					.kind = AST::Type::Qualifier::Kind::Pointer
				}
			);
			next();
		} else if (check(Token::Type::LeftSqBracket)) {
			// array
			next();
			if (!expect(Token::Type::Number, "expected array size")) {
				return {};
			}
			int len = std::stoi(next().text);
			if (!expect(Token::Type::RightSqBracket, "expected ]")) return {};
			next();
			type.qualifiers.push_back(AST::Type::Qualifier{
				.kind = AST::Type::Qualifier::Kind::Array,
				.array_length = len });
		} else {
			break;
		}
	}

	return type;
}
