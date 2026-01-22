#pragma once
#include <string>
#include <memory>
#include <variant>
#include <vector>
#include <optional>

struct Symbol {
	std::optional<std::string> name;
	bool is_assignable{};
};

struct AST {
	using Name = std::string;

#if 0
	/// Hey Lois, I'm an AST! Nyeh heh heh heh
#endif
	using Ptr = std::unique_ptr<AST>;

	struct Type {
		struct Qualifier {
			enum class Kind {
				Pointer,
				Array
			};
			bool is_const{};
			Kind kind{};
			int array_length{};
		};

		Name name;
		std::vector<Qualifier> qualifiers;
	};


	// -- top level --
	struct Function {
		Name name{};
		Type return_type{};
		// paramater list
		Ptr parameter_list{};
		Ptr block{};
	};

	struct ParameterList {
		std::vector<Ptr> items;
	};

	struct Root {
		std::vector<Ptr> functions;
	};

	// -- expressions --
	struct BinaryExpr {
		enum class Kind {
			CmpLesser,
			CmpLesserOrEqual,
			CmpEqual,
			CmpNotEqual,
			CmpGreater,
			CmpGreaterOrEqual,
			CmpAnd,
			CmpOr,
			CmpXor,
			Add,
			Sub,
		};

		Ptr left{}, right{};
		Kind kind;
	};

	struct IdentifierExpr {
		Name name;
	};

	struct LiteralExpr {
		Type type;
		std::string value;
	};

	struct WhileExpr {
		Ptr condition;
		Ptr block;
		Ptr returns;
	};

	struct IfExpr {
		Ptr condition;
		Ptr then_expr;
		Ptr else_expr;
	};

	struct AssignExpr {
		Ptr left;
		Ptr expr;
	};

	struct TupleExpr {
		std::vector<Ptr> exprs;
	};

	struct TupleAssignExpr {
		Ptr tup_left;
		Ptr tup_right;
	};

	// - statements --
	struct BlockStmt {
		std::vector<Ptr> statements;
	};

	struct ReturnStmt {
		Ptr expr;
	};

	struct VariableStmt {
		bool is_const{};
		Name name{};
		Type type{};
		Ptr initializer{};
	};

	struct WhileStmt {
		Ptr condition;
		Ptr block;
	};

	struct IfStmt {
		Ptr condition;
		Ptr then_stmt;
		Ptr else_stmt;
	};

	using Top = std::variant<Function, Root, ParameterList>;

	using Expr = std::variant<
		BinaryExpr,
		IdentifierExpr,
		LiteralExpr,
		AssignExpr,
		WhileExpr,
		IfExpr,
		TupleExpr,
		TupleAssignExpr>;

	using Stmt = std::variant<
		BlockStmt,
		ReturnStmt,
		VariableStmt,
		WhileStmt,
		IfStmt>;

	std::variant<Top, Expr, Stmt> data;
	Symbol sym{};
};
