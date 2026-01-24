#include "x64.hpp"
#include "x64-optimizer.hpp"

enum class CompileTimeComparison {
	Lesser,
	LesserOrEqual,
	Greater,
	GreaterOrEqual,
	Equal,
	NotEqual,
	NotZero,
	IsZero,
	And,
	Or,
	Xor,
};

constexpr static CompileTimeComparison conditional_jump_to_comparison(const X64::MC& mc) {
	if (!mc.is_conditional_jump()) {
		throw std::runtime_error("internal error: machine code not a conditional jump");
	}
	using res = CompileTimeComparison;
	using enum X64::MC::Opcode;

	switch (mc.op) {
		case Jl: return res::Lesser;
		case Jle: return res::LesserOrEqual;
		case Jg: return res::Greater;
		case Jge: return res::GreaterOrEqual;
		case Je: return res::Equal;
		case Jne: return res::NotEqual;
		case Jnz: return res::NotZero;
		case Jz: return res::IsZero;
	}
}

constexpr static bool compile_time_binary_compare(auto l, auto r, CompileTimeComparison cmp) {
	using enum CompileTimeComparison;
	switch (cmp) {
		case Lesser: return l < r;
		case LesserOrEqual: return l <= r;
		case Greater: return l > r;
		case GreaterOrEqual: return l >= r;
		case Equal: return l == r;
		case NotEqual: return l != r;
		case And: return l & r;
		case Or: return l | r;
		case Xor: return l ^ r;
		default:
		throw std::runtime_error("internal error: invalid arguments given for binary comparison");
	}
}


bool X64Optimizer::pass(std::vector<MC>& mc) {
	return (pass_peephole(mc) || pass_unused_labels(mc));
}

bool X64Optimizer::pass_peephole(std::vector<MC>& mc) {
	using enum MC::Opcode;
	using enum Reg;
	bool changed = false;

	for (auto it = mc.begin(); it != mc.end(); ) {
		const auto remaining = [&](size_t n) {
			return std::distance(it, mc.end()) > (static_cast <long> (n));
		};

		const MC& a = *it;

		// Redundant mov elimination
		// mov rax, rax
		if (a.op == MC::Opcode::Mov) {
			if (a.src && *a.src == *a.dst) {
				it = mc.erase(it);
				changed = true;
				continue;
			}
		}

		// Constant comparison via registers
		// mov rX, IMM1
		// mov rY, IMM2
		// mov rax, rX
		// cmp rax, rY
		// jcc T
		// jcc F

		// Disabled in favour of IR optimization
		if (false) if (remaining(5)) {
			auto& b = it[1];
			auto& c = it[2];
			auto& d = it[3];
			auto& e = it[4];

			if (a.op == Mov && a.src->is_imm() &&
				b.op == Mov && b.src->is_imm() &&
				c.op == Mov && c.dst->is_rax() &&
				*c.src == *a.dst &&
				d.op == Cmp &&
				*d.lhs == *c.dst &&
				*d.rhs == *b.dst &&
				e.is_conditional_jump()) {

				const auto l = a.src->imm;
				const auto r = b.src->imm;
				const auto cmp = conditional_jump_to_comparison(e);
				const bool res = compile_time_binary_compare(l, r, cmp);

				MC fold = MC::jmp(*(res ? e.dst : it[5].dst));

				*it = fold;
				mc.erase(it + 1, it + 6);

				changed = true;
				continue;
			}
		}

		// IF TRUE optimization
		// mov rax, 1
		// test rax, rax
		// jnz .LTrue
		// jz .LFalse
		// ->
		// jmp .LTrue
		if (remaining(4)) {
			auto b = it[1];
			auto c = it[2];
			auto d = it[3];

			if (a.op == Mov && a.dst->is_rax() && a.src->is_imm(1) &&
				b.op == Test && b.lhs->is_rax() && b.rhs->is_rax() &&
				c.is_conditional_jump() &&
				d.is_conditional_jump() &&
				d.op == c.negated_jump().op) {
				auto dst = c.dst;
				it = mc.erase(it, it + 3);
				it = mc.insert(it, MC::jmp(*dst));
				changed = true;
				continue;
			}
		}

		// IF FALSE optimization
		// xor rax, rax
		// test rax, rax
		// jnz .LNotZero
		// jz .LZero
		// ->
		// jmp .LZero
		if (remaining(4)) {
			auto b = it[1];
			auto c = it[2];
			auto d = it[3];

			if (a.op == Xor && a.dst->is_rax() && a.src->is_rax() &&
				b.op == Test && b.lhs->is_rax() && b.rhs->is_rax() &&
				c.is_conditional_jump() &&
				d.is_conditional_jump() &&
				d.op == c.negated_jump().op) {
				auto lbl_is_zero = *d.dst;
				it = mc.erase(it, it + 3);
				it = mc.insert(it, MC::jmp(lbl_is_zero));
				changed = true;
				continue;
			}
		}


		// Redundant mov elimination
		// mov rax, X
		// mov Y, rax
		// mov Y, X
		if (remaining(2)) {
			auto nit = std::next(it);
			auto& b = *nit;

			if (a.op == Mov
				&& b.op == Mov
				&& *a.src == *b.src
				&& a.dst->is_rax()
				) {
				const MC fold = MC::mov(*b.dst, *a.src);
				*it = fold;
				mc.erase(nit);
				changed = true;
				continue;
			}
		}


		// Math-shuffle elimination
		// mov A, C
		// add A, B
		// mov C, A
		// is equal to add C, B
		if (remaining(3)) {
			auto& b = it[1];
			auto& c = it[2];

			if (a.op == Mov &&
				b.is_binary_math_operation() &&
				c.op == Mov &&
				a.dst && b.dst && c.src && c.dst &&
				*a.dst == *b.dst &&
				*a.dst == *c.src &&
				*a.src == *c.dst) {

				MC folded{};
				folded.op = b.op;
				folded.dst = c.dst;
				folded.src = b.src;
				*it = folded;
				mc.erase(it + 1, it + 3);
				changed = true;
				continue;
			}
		}

		// Const elimination
		// mov rcx, rbx OR imm
		// add rdx, rcx
		// -> add rdx, rbx OR imm
		if (remaining(1)) {
			auto& b = it[1];
			if (
				a.op == Mov && b.is_binary_math_operation() &&
				a.dst->is_reg() &&
				*a.dst == *b.src
				) {
				const MC folded = MC::add(*b.dst, *a.src);
				*it = folded;
				mc.erase(it + 1);
				changed = true;
				continue;
			}
		}

		// Xor Mov elimination
		// xor rax, rax
		// mov rbx, rax
		// ->
		// xor rbx, rbx
		if (remaining(1)) {
			auto& b = it[1];
			if (a.op == Xor &&
				a.dst == a.src &&
				b.op == Mov && *b.src == *a.dst) {
				MC fold = MC::l_xor(*b.dst, *b.dst);
				*it = fold;
				mc.erase(it + 1);
				changed = true;
				continue;
			}
		}

		// Self-Xor & Cmp Self, 0 fold
		// xor const, const
		// cmp X, const
		// ->
		// cmp X, 0
		if (remaining(1)) {
			auto b = it[1];

			if (a.op == Xor &&
				*a.src == *a.dst &&
				ir.literal_exists(a.src->value_id) &&
				b.op == Cmp &&
				b.lhs->is_reg() && b.rhs == a.src) {
				MC fold = MC::cmp(*b.lhs, Operand::make_imm(0));
				*it = fold;
				mc.erase(it + 1);
				changed = true;
				continue;
			}
		}

		// Known-Zero deterministic jump
		// xor rax, rax
		// cmp rax, 0
		// je L1
		// ->
		// xor rax, rax
		// jmp L1
		if (remaining(3)) {
			auto b = it[1];
			auto c = it[2];
			if (a.op == Xor && a.src == a.dst &&
				b.op == Cmp && b.lhs == a.src &&
				b.rhs->is_imm() && b.rhs->imm == 0 &&
				c.op == Je
				) {
				MC fold = MC::jmp(*c.dst);
				mc.erase(it + 2);
				it[1] = fold;
				changed = true;
				continue;
			}
		}

		// Xor elimination
		// xor rax, rax
		// mov rax, XYZ
		// ->
		// mov rax, 0
		if (remaining(1)) {
			auto b = it[1];
			if (a.op == Xor &&
				a.dst == a.src &&
				b.op == Mov &&
				b.dst == a.src &&
				b.src->is_reg()
				) {
				it = mc.erase(it);
				changed = true;
				continue;
			}
		}


		// Redundant jump removal
		// jmp LX
		// jnz LY
		// -> jmp LX
		if (remaining(1)) {
			auto b = it[1];
			if (a.op == Jmp && b.is_conditional_jump()) {
				mc.erase(it + 1);
				changed = true;
				continue;
			}
		}

		// Redundant jump removal
		// jmp LX
		// LX:
		// -> LX:
		if (remaining(1)) {
			auto b = it[1];
			if (a.op == Jmp && b.op == Label && a.dst->is_imm() && a.dst->imm == b.lbl) {
				it = mc.erase(it);
				changed = true;
				continue;
			}
		}


		// Xor to Zero
		// mov XXX, 0
		// -> xor XXX, XXX
		if (a.op == Mov &&
			a.src->is_imm() &&
			a.src->imm == 0 &&
			!a.dst->is_mem()
			) {
			MC fold = MC::l_xor(*a.dst, *a.dst);
			*it = fold;
			changed = true;
			continue;
		}

		const auto is_mem_zero_mov = [](const MC& ins) {
			return ins.op == Mov &&
				ins.dst->is_mem() &&
				ins.src->is_imm() &&
				ins.src->imm == 0;
		};

		const auto is_self_xor = [&](const MC& mc) {
			return a.op == Xor &&
				a.src->is_reg() &&
				a.dst->is_reg() &&
				a.src->reg == a.dst->reg;
		};


		// Move elimination
		// mov rax, rbx
		// mov rcx, rax
		// -> mov rcx, rbx
		if (remaining(1)) {
			auto b = it[1];
			if (a.op == Mov && b.op == Mov &&
				*a.dst == *b.src) {
				const MC folded = MC::mov(*b.dst, *a.src);
				*it = folded;
				mc.erase(it + 1);
				changed = true;
				continue;
			}
		}


		// Branch optimization
		// - Remove useless setxx
		// Change jnz and jz
		if (remaining(6)) {
			auto b = it[1];
			auto c = it[2];
			auto d = it[3];
			auto e = it[4];
			auto f = it[5];

			if (a.op == Cmp &&
				b.is_setxx() &&
				c.op == MovZx &&
				d.op == Test &&
				e.is_conditional_jump() &&
				f.is_conditional_jump()) {

				MC folds[3]{
					a,
					b.setxx_to_jumpxx(*e.dst),
					b.setxx_to_jumpxx(*f.dst).negated_jump(),
				};

				it[0] = folds[0];
				it[1] = folds[1];
				it[2] = folds[2];
				mc.erase(it + 3, it + 6);

				changed = true;
				continue;
			}
		}

		// Redundant SetXX elimination
		// cmp rbx, rcx
		// sete al
		// movzx rax, al
		// test rax, rax
		// jnz .L0
		// ->
		// cmp rbx, rcx
		// je .L0
		if (remaining(5)) {
			auto b = it[1];
			auto c = it[2];
			auto d = it[3];
			auto e = it[4];

			if (a.op == Cmp &&
				b.is_setxx() &&
				c.op == MovZx &&
				d.op == Test &&
				e.is_conditional_jump()) {

				MC folds[2]{
					a,
					b.setxx_to_jumpxx(*e.dst)
				};

				it[0] = folds[0];
				it[1] = folds[1];
				mc.erase(it + 2, it + 5);

				changed = true;
				continue;
			}
		}

		// Constant comparison elimination
		// 	mov rax, X
		//  cmp rax, Y
		//	jTRUE .L_A
		//	jFALSE .L_B
		// ->
		// if the condition is true:
		// mov rax, X
		// jmp L_TRUE
		// -
		// if the condition is false:
		// mov rax, X
		// jmp L_FALSE

		if (remaining(4)) {
			const auto cmp_mc = it[1];
			const auto jmp_if_true = it[2];
			const auto jmp_if_false = it[3];

			if (a.op == Mov && a.dst->is_rax()  &&
				a.src->is_imm() &&
				cmp_mc.op == Cmp && cmp_mc.lhs->is_rax() && cmp_mc.rhs->is_imm() &&
				jmp_if_true.is_conditional_jump() && jmp_if_false.is_conditional_jump()) {
				const auto l = a.src->imm;
				const auto r = cmp_mc.rhs->imm;
				const auto cmp_type = conditional_jump_to_comparison(jmp_if_true);
				const bool is_cmp_true = compile_time_binary_compare(l, r, cmp_type);
				auto true_dst = *(is_cmp_true ? jmp_if_true.dst : jmp_if_false.dst);

				it = std::next(it);
				it = mc.erase(it, it + 2);
				it = mc.insert(it, MC::jmp(true_dst));

				changed = true;
				continue;
			}
		}

		// Contant elimination
		//	A: mov rcx, 1
		//  B: mov rax, rcx
		// ->
		// mov rax, 1
		if (remaining(1)) {
			auto b = it[1];

			if (a.op == Mov &&
				b.op == Mov &&
				a.src->is_imm() &&
				a.dst->is_reg() &&
				*b.dst == *a.src
				) {

				MC folded = MC::mov(*b.dst, *a.src);

				*it = folded;
				mc.erase(it + 1);

				changed = true;
				continue;
			}
		}

		// RAX elimination
		// mov rax, rcx
		// cmp rax, rdx
		// ->
		// cmp rcx, rdx

		if (remaining(2)) {
			auto b = it[1];
			if (a.op == Mov && a.dst->is_rax() &&
				b.op == Cmp && b.lhs->is_rax()
				) {
				auto old_src = a.src;
				it = mc.erase(it);
				it->lhs = old_src;
				changed = true;
				continue;
			}
		}

		// Redundant jXX removal
		// cmp X, Y
		// J_if_true Ltrue
		// J_if_false Lfalse
		// Ltrue:...
		// ->
		// cmp X, Y
		// jge Lfalse
		// ltrue:
		if (remaining(4)) {
			auto b = it[1];
			auto c = it[2];
			auto d = it[3];

			if (
				a.op == Cmp &&
				b.is_conditional_jump() &&
				c.op == b.negated_jump().op &&
				d.op == Label &&
				*d.lbl == b.dst->imm) {

				it = std::next(it);
				it = mc.erase(it);
				changed = true;
				continue;
			}

		}


		it = std::next(it);
	}

	return changed;
}

bool X64Optimizer::pass_unused_labels(std::vector<MC>& mc) {
	bool changed = false;
	std::unordered_set<int> referenced_labels;

	for (auto& ins : mc) {
		if ((ins.op == MC::Opcode::Jmp || ins.is_conditional_jump()) && ins.dst->is_imm()) {
			if (referenced_labels.contains(ins.dst->imm)) {
				continue;
			} else {
				referenced_labels.insert(ins.dst->imm);
			}
		}
	}

	for (auto it = mc.begin(); it != mc.end(); ) {
		auto& ins = *it;
		if (ins.op == MC::Opcode::Label && !referenced_labels.contains(*ins.lbl)) {
			it = mc.erase(it);
			changed = true;
			continue;
		}

		it = std::next(it);
	}

	return changed;
}

void X64Optimizer::remove_redundant_push_pop(std::vector<MC>& mc) {
	std::unordered_set<Reg> to_remove;
	using enum MC::Opcode;

	for (const auto& ins : mc) {
		if (ins.op == Push) {
			if (ins.src->reg == Reg::rbp) {
				continue;
			}
			if (!to_remove.contains(ins.src->reg)) {
				to_remove.insert(ins.src->reg);
				continue;
			}
		}

		if (ins.op == Pop) continue;

		const auto X = [&](auto& op) {
			if (op && op->is_reg()) {
				auto r = op->reg;
				if (to_remove.contains(op->reg)) {
					to_remove.erase(op->reg);
				}
			}
		};

		X(ins.lhs);
		X(ins.rhs);
		X(ins.src);
		X(ins.dst);
	}

	std::erase_if(mc, [&](auto& ins) {
		if (ins.op == Push && to_remove.contains(ins.src->reg)) {
			return true;
		}
		if (ins.op == Pop && to_remove.contains(ins.src->reg)) {
			return true;
		}
		return false;
	});
}
