#pragma once
#include "ast.hpp"
#include <unordered_map>

using ValueId = int;
using LabelId = ValueId;

constexpr static ValueId NoValue = -1;

struct Value {
	// TODO: language independent type
	AST::Type type;
};

struct Constant {
	std::variant<long /*, double, std::string*/> data;
};

enum class Opcode {
	// Storage
	Alloc,
	Const,
	Store,
	Load,
	// Math
	Add,
	Sub,
	// Comparison
	Lesser,
	LesserOrEqual,
	Greater,
	GreaterOrEqual,
	Equal,
	NotEqual,
	And,
	Or,
	Xor,
	// Control flow
	Label,
	Branch,
	Jump,
	Return,
};

constexpr const char* opcode_name(const Opcode opcode) {
	switch (opcode) {
		case Opcode::Alloc: return "alloc";
		case Opcode::Const: return "const";
		case Opcode::Store: return "store";
		case Opcode::Load: return "load";
		case Opcode::Add: return "add";
		case Opcode::Sub: return "sub";
		case Opcode::Lesser: return "lt";
		case Opcode::LesserOrEqual: return "le";
		case Opcode::Greater: return "gt";
		case Opcode::GreaterOrEqual: return "ge";
		case Opcode::Equal: return "eq";
		case Opcode::NotEqual: return "neq";
		case Opcode::And: return "and";
		case Opcode::Or: return "or";
		case Opcode::Xor: return "xor";
		case Opcode::Label: return "L";
		case Opcode::Branch: return "b";
		case Opcode::Jump: return "j";
		case Opcode::Return: return "ret";
	}
}

struct Inst {
	Opcode opcode;
	ValueId result;
	std::vector<ValueId> operands;

	constexpr auto is_block_terminator() const {
		using enum Opcode;
		switch (opcode) {
			case Branch:
			case Jump:
			case Return:
			return true;
		}
		return false;
	}
};


struct LinearFunction {
	LabelId pro_lbl{};
	LabelId epi_lbl{};
	std::vector<Value> values;
	std::vector<Inst> insts;
	std::unordered_map<std::string, ValueId> locals;
};

struct BasicBlock {
	LabelId lbl_entry;
	std::vector<Inst> inst;
	std::vector<BasicBlock*> successors;
};

struct CFGFunction {
	std::vector<Value> values;
	std::vector<BasicBlock> blocks;
};

struct Module {
	std::unordered_map<std::string, CFGFunction> functions;
};
