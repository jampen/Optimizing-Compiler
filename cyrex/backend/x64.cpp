#include"x64.hpp"
#include "x64-optimizer.hpp"

#include <format>

// To be removed:
struct AllocationStrategy {
	std::optional<X64::ValueLifetime> lifetime = std::nullopt;
	std::vector<bool> consumption{};

	AllocationStrategy(const std::string& strategy) {
		switch (strategy[0]) {
			case 'p': lifetime = X64::ValueLifetime::Persistent; break;
			case 't': lifetime = X64::ValueLifetime::Temporary; break;
			case 's': lifetime = X64::ValueLifetime::Scratch; break;
			case 'x': break;
			default:
			throw std::runtime_error("undefined destination strategy");
		}

		for (int i = 1; i < strategy.size(); ++i) {
			if (strategy[i] == 'y') {
				consumption.push_back(true);
			} else if (strategy[i] == 'n') {
				consumption.push_back(false);
			} else {
				throw std::runtime_error("undefined consumption strategy");
			}
		}
	}
};

const static std::unordered_map<Opcode, AllocationStrategy> alloc_strategy = {
	{Opcode::Alloc,				{"p"}},
	{Opcode::Const,				{"sn"}},
	{Opcode::Store,				{"xnn"}},
	{Opcode::Load,				{"tn"}},
	{Opcode::Add,				{"tnn"}},
	{Opcode::Sub,				{"tnn"}},
	{Opcode::Lesser,			{"tnn"}},
	{Opcode::LesserOrEqual,		{"tnn"}},
	{Opcode::Greater,			{"tnn"}},
	{Opcode::GreaterOrEqual,	{"tnn"}},
	{Opcode::Equal,				{"tnn"}},
	{Opcode::NotEqual,			{"tnn"}},
	{Opcode::And,				{"tnn"}},
	{Opcode::Or,				{"tnn"}},
	{Opcode::Xor,				{"tnn"}},
	{Opcode::Label,				{"xn"}},
	{Opcode::Branch,			{"xnnn"}},
	{Opcode::Jump,				{"xn"}},
	{Opcode::Return,			{"xn"}},
};

X64::X64(IRGen& ir, X64Optimizer& optimizer) : ir(ir), optimizer(optimizer) {}


void X64::module() {
	function_textstream << "bits 64\n";
	function_textstream << "section .text\n";

	for (const auto& [fn_name, fn] : ir.get_functions()) {
		function_textstream << "global " << fn_name << '\n';
		function(fn_name, fn);
	}
}

void X64::function(const std::string& name, const CFGFunction& fn) {
	const auto align_16 = [](const std::size_t value) {
		return (value + 15) & ~std::size_t(15);
	};

	function_mc = {};
	function_mc.epi_lbl = fn.blocks.back().lbl_entry;

	// generate machine code
	for (const auto& bb : fn.blocks) {
		for (const auto& inst : bb.inst) {
			instruction(function_mc.block, inst);
		}
	}

	const int ss = align_16(function_mc.stack_size);

	const auto gen_prologue = [&]() {
		if (ss) {
			function_mc.claimed_callee_saved_regs.emplace(Reg::rbp);
			function_mc.prologue.push_back(MC::push(reg(Reg::rbp)));
			function_mc.prologue.push_back(MC::mov(reg(Reg::rbp), reg(Reg::rsp)));
		}

		for (const auto r : function_mc.regs_to_restore) {
			function_mc.prologue.push_back(MC::push(reg(r)));
		}

		if (ss) {
			function_mc.prologue.push_back(MC::sub(reg(Reg::rsp), Operand::make_imm(ss)));
		}
	};


	const auto gen_epilogue = [&]() {
		for (auto it = function_mc.regs_to_restore.rbegin(); it != function_mc.regs_to_restore.rend(); it = std::next(it)) {
			function_mc.epilogue.push_back(MC::pop(reg(*it)));
		}
		if (ss) {
			function_mc.epilogue.push_back(MC::pop(reg(Reg::rbp)));
			function_mc.epilogue.push_back(MC::add(reg(Reg::rsp), Operand::make_imm(ss)));
		}
		function_mc.epilogue.push_back(MC::ret());
	};

	gen_prologue();
	gen_epilogue();

	std::vector<MC> final_mc;
	final_mc.reserve(function_mc.prologue.size() + function_mc.block.size() + function_mc.epilogue.size());
	final_mc.insert(final_mc.end(), function_mc.prologue.begin(), function_mc.prologue.end());
	final_mc.insert(final_mc.end(), function_mc.block.begin(), function_mc.block.end());
	final_mc.insert(final_mc.end(), function_mc.epilogue.begin(), function_mc.epilogue.end());

	// Optimization
	optimize(final_mc);

	function_textstream << name << ":\n";
	emit(function_textstream, final_mc);
	function_textstream << "\tret\n";
	function_mc = {};
}

void X64::instruction(std::vector<MC>& mc, const Inst& inst) {
	const auto& strat = alloc_strategy.at(inst.opcode);

	if (strat.lifetime && *strat.lifetime != ValueLifetime::Scratch) {
		alloc_on_demand(inst.result);
	}

	const auto push_mc = [&](auto ins) {
		mc.push_back(ins);
	};

	using enum Reg;
	using enum Opcode;
	const auto result = [&]() { return operand(inst.result); };
	const auto inst_operand = [&](int index) { return operand(inst.operands[index]); };
	const auto src = [&]() { return inst_operand(0); };
	const auto lhs = [&]() { return inst_operand(0); };
	const auto rhs = [&]() { return inst_operand(1); };

	const auto cmp = [&](auto&& set_reg_to_result) {
		push_mc(MC::mov(reg(rax), lhs()));
		push_mc(MC::cmp(reg(rax), rhs()));
		push_mc(set_reg_to_result(reg(al)));
		push_mc(MC::movzx(reg(rax), reg(al)));
		push_mc(MC::mov(result(), reg(rax)));
	};

	switch (inst.opcode) {
		case Alloc: break;
		case Const: {
			/*
			const auto& v = ir.constants.at(inst.result);
			std::visit([&](auto&& val) {
				//push_mc(MC::mov(operand(inst.result), Operand::make_imm(val)));
			}, v.data);
			*/
		} break;
		case Store: push_mc(MC::mov(inst_operand(0), inst_operand(1)));  break;
		case Load:
		push_mc(MC::mov(reg(rax), inst_operand(0)));
		push_mc(MC::mov(result(), reg(rax)));
		break;
		case Add:
		push_mc(MC::mov(reg(rax), inst_operand(0)));
		push_mc(MC::add(reg(rax), inst_operand(1)));
		push_mc(MC::mov(result(), reg(rax)));
		break;
		case Sub:
		push_mc(MC::mov(reg(rax), inst_operand(0)));
		push_mc(MC::sub(reg(rax), inst_operand(1)));
		push_mc(MC::mov(result(), reg(rax)));
		break;
		case Lesser: cmp(MC::setl); break;
		case LesserOrEqual: cmp(MC::setle); break;
		case Greater: cmp(MC::setg); break;
		case GreaterOrEqual: cmp(MC::setge); break;
		case Equal: cmp(MC::sete); break;
		case NotEqual: cmp(MC::setne); break;
		case And: break;
		case Or: break;
		case Xor: break;
		case Label: push_mc(MC::label(inst.operands[0])); break;
		case Branch:
		push_mc(MC::mov(reg(rax), src()));
		push_mc(MC::test(reg(rax), reg(rax)));
		push_mc(MC::jnz(Operand::make_imm(inst.operands[1])));
		push_mc(MC::jz(Operand::make_imm(inst.operands[2])));
		break;
		case Jump:
		push_mc(MC::jmp(Operand::make_imm(inst.operands[0])));
		break;
		case Return:
		if (inst.operands[0] != NoValue) {
			push_mc(MC::mov(reg(rax), operand(inst.operands[0])));
		}
		push_mc(MC::jmp(Operand::make_imm(function_mc.epi_lbl)));
		break;
	}

	// consumption
	for (int i = 0; i < inst.operands.size(); ++i) {
		if (strat.consumption[i]) {
			consume(inst.operands[i]);
		}
	}
}

void X64::optimize(std::vector<MC>& mc) {
	while (optimizer.pass(mc)) {}
	optimizer.remove_redundant_push_pop(mc);
}

std::string X64::assembly() const {
	return function_textstream.str();
}

X64::Operand X64::operand(const ValueId value_id) {
	if (locations.contains(value_id)) {
		return location(value_id);
	}

	if (ir.literal_exists(value_id)) {
		return constant(value_id);
	}
	throw std::runtime_error("internal error: use of unallocated value");
}

X64::Operand X64::constant(const ValueId constant_value_id) {
	// It must be a constant
	return std::visit([&](auto&& v) {
		return Operand::make_imm((int64_t)v, constant_value_id);
	}, ir.get_literal_by_id(constant_value_id).data);
}


X64::Operand X64::location(const ValueId value_id) {
	const auto& loc = locations.at(value_id);
	if (loc.kind == ValueLocation::Kind::Reg) {
		return Operand::make_reg((Reg)loc.loc.reg, value_id);
	} else {
		return Operand::make_mem(loc.loc.stack, value_id);
	}
}

std::string X64::emit(const Operand& op) {
	switch (op.kind) {
		case Operand::Kind::Reg:
		return reg_to_string(op.reg);
		case Operand::Kind::Mem:
		return std::format("[rbp-{}]", op.stack);
		case Operand::Kind::Imm:
		return std::to_string(op.imm);
	}
	//std::unreachable();
}

void X64::emit(std::ostream& ts, const std::vector<MC>& mc) {
	using enum MC::Opcode;
	using std::format;

	for (const auto& ins : mc) {
		switch (ins.op) {
			// Mov
			case Mov:
			if (ins.dst->value_id != NoValue && ins.dst->is_mem()) {
				ts << format("\tmov {} {}, {}\n", type_size(ir.get_value_by_id(ins.dst->value_id).type).str(), emit(*ins.dst), emit(*ins.src)); break;
			} else {
				ts << format("\tmov {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
			}
			break;
			case Push: ts << format("\tpush {}\n", emit(*ins.src)); break;
			case Pop: ts << format("\tpop {}\n", emit(*ins.src)); break;
			case MovZx:	ts << format("\tmovzx {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
				// Math
			case Add:	ts << format("\tadd {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
			case Sub:	ts << format("\tsub {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
			case Inc:	ts << format("\tinc {}\n", emit(*ins.src)); break;
			case Dec:	ts << format("\tdec {}\n", emit(*ins.src)); break;
				// Logic
			case And:	ts << format("\tand {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
			case Or:	ts << format("\tor {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
			case Xor:	ts << format("\txor {}, {}\n", emit(*ins.dst), emit(*ins.src)); break;
				// Cmps
			case Cmp:	ts << format("\tcmp {}, {}\n", emit(*ins.lhs), emit(*ins.rhs)); break;
			case Test:	ts << format("\ttest {}, {}\n", emit(*ins.lhs), emit(*ins.rhs)); break;
				// Setxx
			case Setl:	ts << format("\tsetl {}\n", emit(*ins.dst)); break;
			case Setle:	ts << format("\tsetle {}\n", emit(*ins.dst)); break;
			case Setg:	ts << format("\tsetg {}\n", emit(*ins.dst)); break;
			case Setge:	ts << format("\tsetge {}\n", emit(*ins.dst)); break;
			case Sete:	ts << format("\tsete {}\n", emit(*ins.dst)); break;
			case Setne:	ts << format("\tsetne {}\n", emit(*ins.dst)); break;
				// Jumps
			case Jmp:	ts << format("\tjmp .L{}\n", emit(*ins.dst)); break;
			case Jnz:	ts << format("\tjnz .L{}\n", emit(*ins.dst)); break;
			case Jz:	ts << format("\tjz .L{}\n", emit(*ins.dst)); break;
			case Jl:	ts << format("\tjl .L{}\n", emit(*ins.dst)); break;
			case Jle:	ts << format("\tjle .L{}\n", emit(*ins.dst)); break;
			case Jg:	ts << format("\tjg .L{}\n", emit(*ins.dst)); break;
			case Jge:	ts << format("\tjge .L{}\n", emit(*ins.dst)); break;
			case Je:	ts << format("\tje .L{}\n", emit(*ins.dst)); break;
			case Jne:	ts << format("\tjne .L{}\n", emit(*ins.dst)); break;
				// Decl
			case Label: ts << format(".L{}:\n", *ins.lbl); break;
			case Ret:
			if (ins.src) {
				ts << format("\tmov rax, {}\n", emit(*ins.src));
				ts << format("\tjmp .L{}\n", function_mc.epi_lbl);
			}
			break;
			case Nop:	ts << "\tnop\n"; break;
		}
	}
}