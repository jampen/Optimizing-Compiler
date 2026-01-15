#include "x64.hpp"

void X64::optimize() {
	while (
		pass_peephole() ||
		pass_unused_labels()
		) {
	}
}


bool X64::pass_peephole() {
	bool changed = false;

	using enum MC::Opcode;
	using enum Reg;

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
				&& a.dst->is_reg()
				&& a.dst->reg == rax
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
		if (false) if (remaining(1)) {
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

		// Xor const to cmp folding
		// xor const, const
		// cmp X, const
		// ->
		// cmp X, 0
		if (remaining(1)) {
			auto b = it[1];

			if (a.op == Xor &&
				*a.src == *a.dst &&
				ir.is_constant(a.src->value_id) &&
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


		// Add -> inc
		// add rax, 1
		// -> inc rax
		if (false) if (a.op == Add && a.src->is_imm() && a.src->imm == 1) {
			*it = MC::inc(*a.dst);
			changed = true;
			continue;
		}
		// Sub to dec
		// sub rax, 1
		// -> dec rax
		if (false) if (a.op == Dec && a.src->is_imm() && a.src->imm == 1) {
			*it = MC::dec(*a.dst);
			changed = true;
			continue;
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

		it = std::next(it);
	}

	return changed;
}

bool X64::pass_unused_labels() {
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
