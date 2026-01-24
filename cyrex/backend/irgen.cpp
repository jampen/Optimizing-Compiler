#include "irgen.hpp"
#include <utility>
#include <iostream>
#include <format>

template<class ...Ts>
struct overloaded : Ts... { using Ts::operator()...; };

ValueId IRGen::gen(const AST::Ptr& ptr) {
	auto visitor = overloaded{
		[&](const AST::Top& x) { return top(x); },
		[&](const AST::Stmt& x) { return stmt(x); },
		[&](const AST::Expr& x) { return expr(x); }
	};
	return std::visit(visitor, ptr->data);
}

const Value& IRGen::get_value_by_id(const ValueId value_id) const {
	return values.at(value_id);
}

const Literal& IRGen::get_literal_by_id(const ValueId value_id) const {
	return literals.at(value_id);
}

const CFGFunction& IRGen::get_function_by_name(const std::string& name) const {
	return mod.functions.at(name);
}

bool IRGen::literal_exists(const ValueId value_id) const {
	return literals.contains(value_id);
}

ValueId IRGen::top(const AST::Top& top) {
	auto visitor = overloaded{
		[&](const AST::Root& x) { return root(x); },
		[&](const AST::Function& x) { return function(x); },
		[&](const AST::ParameterList& x) { return parameter_list(x); }
	};
	return std::visit(visitor, top);
}

ValueId IRGen::root(const AST::Root& root) {
	for (const auto& fn : root.functions) {
		gen(fn);
	}

	for (const auto& [fn_name, fn] : functions) {
		auto blocks = bbg.function(fn);
		bbg.link_blocks(blocks, fn);
		bbg.fn_to_bbs[fn_name] = blocks;
	}

	// Generate basic blocks
	for (const auto& [fn_name, fn] : functions) {
		const auto& bbs = bbg.fn_to_bbs.at(fn_name);
		auto& mf = mod.functions[fn_name];
		mf.blocks = bbs;
		mf.values = fn.values;
	}

	return NoValue;
}

ValueId IRGen::stmt(const AST::Stmt& stmt) {
	auto visitor = overloaded{
		[&](const AST::IfStmt& x) { return if_stmt(x); },
		[&](const AST::WhileStmt& x) { return while_stmt(x); },
		[&](const AST::BlockStmt& x) { return block_stmt(x); },
		[&](const AST::ReturnStmt& x) { return return_stmt(x); },
		[&](const AST::VariableStmt& x) { return var_stmt(x); }
	};
	return std::visit(visitor, stmt);
}

ValueId IRGen::expr(const AST::Expr& expr) {
	auto visitor = overloaded{
		[&](const AST::LiteralExpr& x) { return literal_expr(x); },
		[&](const AST::IfExpr& x) { return if_expr(x);  },
		[&](const AST::WhileExpr& x) { return while_expr(x);  },
		[&](const AST::BinaryExpr& x) { return binary_expr(x);  },
		[&](const AST::AssignExpr& x) { return assign_expr(x);  },
		[&](const AST::IdentifierExpr& x) {return identifier_expr(x); },
		[&](const AST::TupleExpr& x) { return NoValue;  },
		[&](const AST::TupleAssignExpr& x) { return NoValue;  },
	};
	return std::visit(visitor, expr);
}

ValueId IRGen::function(const AST::Function& function) {
	if (mod.functions.contains(function.name)) {
		push_error(std::format("function {} is already defined", function.name));
		return NoValue;
	}
	current_fn = &functions[function.name];
	current_fn->pro_lbl = new_label();
	push_label(current_fn->pro_lbl);
	current_fn->epi_lbl = new_label();
	gen(function.block);
	push_label(current_fn->epi_lbl);
	return NoValue;
}

ValueId IRGen::parameter_list(const AST::ParameterList& parameter_list) {
	for (const auto& param : parameter_list.items) {
		gen(param);
	}
	return NoValue;
}

ValueId IRGen::if_stmt(const AST::IfStmt& if_stmt) {
	LabelId l_true = new_label();
	LabelId l_false = new_label();
	LabelId l_done = new_label();
	ValueId condres = gen(if_stmt.condition);

	push_inst(Opcode::Branch, NoValue, { condres, l_true, l_false });

	// TEST cmpres....
	push_label(l_true);
	gen(if_stmt.then_stmt);

	if (if_stmt.else_stmt) {
		push_inst(Opcode::Jump, NoValue, { l_done });
		push_label(l_false);
		gen(if_stmt.else_stmt);
	}

	if (!if_stmt.else_stmt) {
		push_label(l_false);
	}

	push_inst(Opcode::Jump, NoValue, { l_done });
	push_label(l_done);
	return NoValue;
}

ValueId IRGen::while_stmt(const AST::WhileStmt& while_stmt) {
	LabelId l_cond = new_label();
	LabelId l_body = new_label();
	LabelId l_exit = new_label();

	push_label(l_cond);
	ValueId cmpres = gen(while_stmt.condition);
	push_inst(Opcode::Branch, NoValue, { cmpres, l_body, l_exit });
	push_label(l_body);
	gen(while_stmt.block);
	push_inst(Opcode::Jump, NoValue, { l_cond });
	push_label(l_exit);
	return NoValue;
}

ValueId IRGen::block_stmt(const AST::BlockStmt& block) {
	enter_scope();
	for (const auto& stmt : block.statements) {
		gen(stmt);
	}
	exit_scope();
	return NoValue;
}

ValueId IRGen::return_stmt(const AST::ReturnStmt& ret) {
	ValueId resid = NoValue;
	if (ret.expr) {
		resid = gen(ret.expr);
	}
	push_inst(Opcode::Return, NoValue, { resid });
	return resid;
}

ValueId IRGen::var_stmt(const AST::VariableStmt& var) {
	auto& syms = scopes.back().symbols;

	if (syms.contains(var.name)) {
		push_error(std::format("variable {} already defined in scope", var.name));
		return NoValue;
	}

	ValueId vid = new_value(var.type);
	push_inst(Opcode::Alloc, vid);

	if (var.initializer) {
		ValueId init = gen(var.initializer);
		push_inst(Opcode::Store, NoValue, { vid, init });
	}

	syms[var.name] = vid;
	return NoValue;
}

ValueId IRGen::if_expr(const AST::IfExpr& if_expr) {
	LabelId l_done = new_label();
	LabelId l_true = new_label();
	LabelId l_false = new_label();
	ValueId condres = gen(if_expr.condition);

	push_inst(Opcode::Branch, NoValue, { condres, l_true, l_false });

	push_label(l_true);
	ValueId val_iftrue = gen(if_expr.then_expr);
	ValueId result = new_value(values[val_iftrue].type);
	push_inst(Opcode::Load, result, { val_iftrue });
	push_inst(Opcode::Jump, NoValue, { l_done });

	push_label(l_false);
	ValueId val_iffalse = gen(if_expr.else_expr);
	push_inst(Opcode::Load, result, { val_iffalse });

	push_inst(Opcode::Jump, NoValue, { l_done });
	push_label(l_done);
	return result;
}

ValueId IRGen::while_expr(const AST::WhileExpr& while_expr) {
	LabelId l_cond = new_label();
	LabelId l_body = new_label();
	LabelId l_exit = new_label();

	push_inst(Opcode::Jump, NoValue, { l_cond });

	push_label(l_cond);
	ValueId cond = gen(while_expr.condition);
	push_inst(Opcode::Branch, NoValue, { cond, l_body, l_exit });

	push_label(l_body);
	gen(while_expr.block);
	push_inst(Opcode::Jump, NoValue, { l_cond });

	push_label(l_exit);
	ValueId ret_expr = gen(while_expr.returns);
	return ret_expr;
}

ValueId IRGen::literal_expr(const AST::LiteralExpr& literal) {
	ValueId value = new_value(literal.type);
	push_inst(Opcode::Const, value);
	literals[value] = parse_literal(literal);
	return value;
}

ValueId IRGen::identifier_expr(const AST::IdentifierExpr& identifier) {
	auto maybe_id = find_symbol(identifier.name);
	if (maybe_id == std::nullopt) {
		push_error(std::format("symbol {} is undefined", identifier.name));
		return NoValue;
	} else {
		return *maybe_id;
	}
}

ValueId IRGen::binary_expr(const AST::BinaryExpr& binary) {
	auto left = gen(binary.left);
	auto res = new_value(values.at(left).type);
	auto right = gen(binary.right);

	switch (binary.kind) {
		case AST::BinaryExpr::Kind::Add:	push_inst(Opcode::Add, res, { left, right }); break;
		case AST::BinaryExpr::Kind::Sub:	push_inst(Opcode::Sub, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpAnd: push_inst(Opcode::And, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpOr:	push_inst(Opcode::Or, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpXor:	push_inst(Opcode::Xor, res, { left, right }); break;

		case AST::BinaryExpr::Kind::CmpLesser:			push_inst(Opcode::Lesser, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpLesserOrEqual:	push_inst(Opcode::LesserOrEqual, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpEqual:			push_inst(Opcode::Equal, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpNotEqual:		push_inst(Opcode::NotEqual, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpGreater:			push_inst(Opcode::Greater, res, { left, right }); break;
		case AST::BinaryExpr::Kind::CmpGreaterOrEqual:	push_inst(Opcode::GreaterOrEqual, res, { left, right }); break;
	}
	return res;
}

ValueId IRGen::assign_expr(const AST::AssignExpr& assign) {
	const ValueId leftval = gen(assign.left);
	const ValueId exprval = gen(assign.expr);
	push_inst(Opcode::Store, NoValue, { leftval, exprval });
	return leftval;
}

Literal IRGen::parse_literal(const AST::LiteralExpr literal) {
	if (literal.type.name == "int") {
		return { std::stol(literal.value) };
	}
	if (literal.type.name == "long") {
		return { std::stol(literal.value) };
	}
	if (literal.type.name == "double") {
		throw "double not implemented";
		//return { std::stod(literal.value) };
	}
	if (literal.type.name == "string") {
		//return { literal.value };
		throw "string not implemented";
	}
	push_error("unsupported literal type");
	return {};
}

void IRGen::push_error(const std::string& error_message) {
	errors.push_back(error_message);
}

void IRGen::push_inst(const Opcode o, const ValueId result, const std::vector<ValueId>& operands) {
	current_fn->insts.emplace_back(o, result, operands);
}

void IRGen::push_label(const LabelId label_id) {
	push_inst(Opcode::Label, NoValue, { label_id });
}

ValueId IRGen::new_value(const AST::Type& type) {
	const auto id = values.size();
	Value value;
	value.type = type;
	values.push_back(value);
	return id;
}

void IRGen::enter_scope() {
	scopes.emplace_back();
}

std::optional<ValueId> IRGen::find_symbol(const std::string& name) {
	for (auto it = scopes.rbegin(); it != scopes.rend(); it = std::next(it)) {
		if (auto s_it = it->symbols.find(name); s_it != it->symbols.end()) {
			auto& [s_name, s_id] = *s_it;
			return s_id;
		}
	}
	return std::nullopt;
}

void IRGen::exit_scope() {
	scopes.pop_back();
}

LabelId IRGen::new_label() {
	return next_label_id++;
}

std::vector<BasicBlock> BasicBlockGenerator::function(const LinearFunction& fn) {
	std::vector<BasicBlock> blocks;
	bool push_block = true;

	BasicBlock* bb{};
	for (std::size_t i = 0; i < fn.insts.size(); ++i) {
		auto& ins = fn.insts[i];

		if (push_block) {
			bb = &blocks.emplace_back();
			push_block = false;

			if (ins.opcode == Opcode::Label) {
				bb->lbl_entry = ins.operands[0];
			} else {
				throw std::runtime_error("internal error: BB's should always begin with a label");
			}
		}

		bb->inst.push_back(ins);

		if (ins.is_block_terminator()) {
			push_block = true;
			continue;
		}

		if (i + 1 != fn.insts.size()) {
			const auto& n = fn.insts[i + 1];
			if (n.opcode == Opcode::Label && n.operands[0] != bb->lbl_entry) {
				push_block = true;

			}
		}
	}

	return blocks;
}

void BasicBlockGenerator::link_blocks(std::vector<BasicBlock>& blocks, const LinearFunction& fn) {
	std::unordered_map<LabelId, BasicBlock*> lbl_to_bb;

	using enum Opcode;

	for (auto& bb : blocks) {
		for (const auto& ins : bb.inst) {
			if (ins.opcode == Label) {
				lbl_to_bb[ins.operands[0]] = &bb;
				continue;
			}
		}
	}

	// Look at the last instruction to connect the blocks
	for (std::size_t i = 0; i < blocks.size(); ++i) {
		auto& bb = blocks[i];
		if (bb.inst.empty()) continue;
		auto& ins = bb.inst.back();

		if (ins.opcode == Jump) {
			bb.successors.push_back(lbl_to_bb.at(ins.operands[0]));
			continue;
		} else if (ins.opcode == Branch) {
			bb.successors.push_back(lbl_to_bb.at(ins.operands[1]));
			bb.successors.push_back(lbl_to_bb.at(ins.operands[2]));
			continue;
		} else if (ins.opcode == Return) {
			bb.successors.push_back(lbl_to_bb.at(fn.epi_lbl));
			continue;
		} else {
			// Insert a fallthrough if we are at the end of the block
			// and it has no terminator.
			if (i + 1 < blocks.size()) {
				bb.inst.push_back(Inst{ Jump, NoValue, {blocks[i + 1].lbl_entry} });
				bb.successors.push_back(&blocks[i + 1]);
			}
		}
	}
}

const std::vector<BasicBlock>& BasicBlockGenerator::get_fn_bbs(const std::string& function_name) const {
	return fn_to_bbs.at(function_name);
}