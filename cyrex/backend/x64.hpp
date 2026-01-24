#pragma once
#include "irgen.hpp"

#include <sstream>
#include <array>
#include <unordered_set>

struct X64Optimizer;

class X64 {
public:

	enum class Reg {
		rbp, rsp, rax, rbx, rcx, rdx, /*  */ r8, r9, r10, r11, r12, r13, r14, r15,
		ebp, esp, eax, ebx, ecx, edx, /*  */ r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d,
		bp, sp, ax, bx, cx, dx, /*      */ r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w,
		bpl, spl, al, bl, cl, dl, /*      */ r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b,
	};
	
	enum class RegSize { Qword = 0, Dword = 1, Word = 2, Byte = 3 };

	constexpr static int num_qword_regs = 14;

	struct TypeSize {
		RegSize elem_size{};
		int num_bytes{};
		bool is_array{};

		constexpr const char* str() const {
			switch (elem_size) {
				case RegSize::Byte: return "byte";
				case RegSize::Word: return "word";
				case RegSize::Dword: return "dword";
				case RegSize::Qword: return "qword";
			}
		}
	};

	// System dependent!
	constexpr static std::array volatile_regs = { Reg::rax, Reg::rcx, Reg::rdx, Reg::r8, Reg::r9, Reg::r10, Reg::r11 };
	constexpr static std::array callee_saved_regs = { Reg::rbx, Reg::r12, Reg::r13, Reg::r14, Reg::r15 };

	constexpr static Reg to_largest_reg(const Reg reg) {
#define X(q,d,w,b) case Reg::q: case Reg::d: case Reg::w: case Reg::b: return Reg::q
		switch (reg) {
			X(rbp, ebp, bp, bpl);
			X(rsp, esp, sp, spl);
			X(rax, eax, ax, al);
			X(rbx, ebx, bx, bl);
			X(rcx, ecx, cx, cl);
			X(rdx, edx, dx, dl);
			X(r8, r8d, r8w, r8b);
			X(r9, r9d, r9w, r9b);
			X(r10, r10d, r10w, r10b);
			X(r11, r11d, r11w, r11b);
			X(r12, r12d, r12w, r12b);
			X(r13, r13d, r13w, r13b);
			X(r14, r14d, r14w, r14b);
			X(r15, r15d, r15w, r15b);
		}
#undef X
	}

	constexpr static bool is_reg_callee_saved(const Reg reg) {
		const auto promoted = to_largest_reg(reg);
		for (const auto csr : callee_saved_regs) {
			if (csr == promoted) return true;
		}
		return false;
	}


	constexpr const char* reg_to_string(const Reg reg) {
#define X(reg) case Reg::reg: return #reg
		switch (reg) {
			X(rsp); X(esp); X(sp); X(spl);
			X(rbp); X(ebp); X(bp); X(bpl);
			X(rax); X(eax);  X(ax); X(al);
			X(rbx); X(ebx);  X(bx); X(bl);
			X(rcx); X(ecx);  X(cx); X(cl);
			X(rdx); X(edx);  X(dx); X(dl);
			X(r8);  X(r8d);  X(r8w); X(r8b);
			X(r9);  X(r9d);  X(r9w); X(r9b);
			X(r10); X(r10d); X(r10w); X(r10b);
			X(r11); X(r11d); X(r11w); X(r11b);
			X(r12); X(r12d); X(r12w); X(r12b);
			X(r13); X(r13d); X(r13w); X(r13b);;
			X(r14); X(r14d); X(r14w); X(r14b);
			X(r15); X(r15d); X(r15w); X(r15b);
		}
#undef X
	}

	constexpr static Reg same_reg_of_diff_size(const Reg reg, const RegSize size) {
		const auto largest = to_largest_reg(reg);
		const int reg_as_int = (int)reg;
		return (Reg)(reg_as_int + num_qword_regs * (int)size);
	}

	// TODO: Use semantic analysis!!!

	enum class ValueLifetime {
		Temporary,
		Persistent,
		Scratch
	};

	struct ValueLocation {
		enum class Kind {
			Stack, Reg
		};
		Kind kind{};
		union { int stack; Reg reg; } loc{};
		ValueLifetime lifetime{};
	};

	struct Operand {
		enum class Kind {
			Reg,
			Mem,
			Imm,
		};

		Kind kind{};

		union {
			Reg reg;
			std::size_t stack;
			std::int64_t imm;
		};

		ValueId value_id{ NoValue };

		constexpr static Operand make_reg(const Reg r, const ValueId vid) {
			return { Kind::Reg, {.reg = r}, vid };
		}

		constexpr static Operand make_mem(const std::size_t stack, const ValueId vid) {
			return { Kind::Mem, {.stack = stack }, vid };
		}

		constexpr static Operand make_imm(const std::int64_t imm, const ValueId vid = NoValue) {
			return { Kind::Imm, {.imm = imm}, vid };
		}

		constexpr bool is_reg() const {
			return kind == Kind::Reg;
		}

		constexpr bool is_imm() const {
			return kind == Kind::Imm;
		}

		constexpr bool is_imm(const int64_t val) const {
			return is_imm() && imm == val;
		}

		constexpr bool is_mem() const {
			return kind == Kind::Mem;
		}

		constexpr bool is_rax() const {
			return is_reg() && reg == Reg::rax;
		}


		constexpr bool operator == (const Operand& other) const {
			if (kind != other.kind) return false;

			switch (kind) {
				case Kind::Reg: return reg == other.reg;
				case Kind::Mem: return stack == other.stack;
				case Kind::Imm: return imm == other.imm;
			}
		}
	};

	constexpr static X64::Operand reg(const X64::Reg r, const ValueId value_id = NoValue) {
		return X64::Operand::make_reg(r, value_id);
	};

	struct MC {
		enum class Opcode {
			// Storage
			Mov, MovZx, Push, Pop,
			// Maths
			Add, Sub,
			Inc, Dec,
			// Logic
			And, Or, Xor,
			// Comparisons
			Cmp, Test,
			Setl, Setle, Setg, Setge, Sete, Setne,
			// Branching
			Jmp,
			Jl,
			Jle,
			Jg,
			Jge,
			Je,
			Jne,
			Jnz,
			Jz,
			Label,
			Ret,
			Nop
		};

		Opcode op{};
		std::optional<Operand> dst = std::nullopt;
		std::optional<Operand> src = std::nullopt;
		std::optional<Operand> lhs = std::nullopt;
		std::optional<Operand> rhs = std::nullopt;
		std::optional<int> lbl = std::nullopt;

		constexpr bool is_binary_math_operation() const noexcept {
			using enum Opcode;
			switch (op) {
				case Add:
				case Sub:
				return true;
			}
			return false;
		}

		constexpr bool is_setxx() const noexcept {
			using enum Opcode;
			switch (op) {
				case Setl:
				case Setle:
				case Setg:
				case Setge:
				case Sete:
				case Setne:
				return true;
			}
			return false;
		}

		constexpr bool is_conditional_jump() const {
			using enum Opcode;
			switch (op) {
				case Jl:
				case Jle:
				case Jg:
				case Jge:
				case Je:
				case Jne:
				case Jnz:
				case Jz:
				return true;
			}
			return false;
		}

		constexpr MC setxx_to_jumpxx(const Operand& dst) const {
			if (!is_setxx()) {
				throw std::runtime_error("internal error: instruction is not setxx");
			}
			using enum Opcode;
			switch (op) {
				case Setl:  return jl(dst);
				case Setle: return jle(dst);
				case Setg:  return jg(dst);
				case Setge: return jge(dst);
				case Sete:  return je(dst);
				case Setne: return jne(dst);
			}
		}

		constexpr MC negated_jump() const {
			if (!is_conditional_jump()) {
				throw std::runtime_error("internal error: jmp instruction is not conditional");
			}
			using enum Opcode;
			switch (op) {
				case Jl: return jge(*dst);
				case Jle: return jg(*dst);
				case Jg: return jle(*dst);
				case Jge: return jl(*dst);
				case Jz: return jnz(*dst);
				case Jnz: return jz(*dst);
				case Je: return jne(*dst);
				case Jne: return je(*dst);
			}
		}

		constexpr static MC mov(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::Mov, .dst = dst, .src = src };
		}

		constexpr static MC movzx(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::MovZx, .dst = dst, .src = src };
		}

		constexpr static MC push(const Operand& src) {
			return MC{ .op = Opcode::Push, .src = src };
		}

		constexpr static MC pop(const Operand& src) {
			return MC{ .op = Opcode::Pop, .src = src };
		}

		// Maths
		constexpr static MC add(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::Add, .dst = dst, .src = src };
		}

		constexpr static MC sub(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::Sub, .dst = dst, .src = src };
		}

		constexpr static MC inc(const Operand& src) {
			return MC{ .op = Opcode::Inc, .src = src };
		}

		constexpr static MC dec(const Operand& src) {
			return MC{ .op = Opcode::Dec, .src = src };
		}

		// Logic
		constexpr static MC l_and(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::And, .dst = dst, .src = src };
		}

		constexpr static MC l_or(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::Or, .dst = dst, .src = src };
		}

		constexpr static MC l_xor(const Operand& dst, const Operand& src) {
			return MC{ .op = Opcode::Xor, .dst = dst, .src = src };
		}

		// Comparisons
		constexpr static MC cmp(const Operand& lhs, const Operand& rhs) {
			return MC{ .op = Opcode::Cmp, .lhs = lhs, .rhs = rhs };
		}

		constexpr static MC test(const Operand& lhs, const Operand& rhs) {
			return MC{ .op = Opcode::Test, .lhs = lhs, .rhs = rhs };
		}

		constexpr static MC setl(const Operand& dst) {
			return MC{ .op = Opcode::Setl, .dst = dst };
		}

		constexpr static MC setle(const Operand& dst) {
			return MC{ .op = Opcode::Setle, .dst = dst };
		}

		constexpr static MC setg(const Operand& dst) {
			return MC{ .op = Opcode::Setg, .dst = dst };
		}

		constexpr static MC setge(const Operand& dst) {
			return MC{ .op = Opcode::Setge, .dst = dst };
		}

		constexpr static MC sete(const Operand& dst) {
			return MC{ .op = Opcode::Sete, .dst = dst };
		}

		constexpr static MC setne(const Operand& dst) {
			return MC{ .op = Opcode::Setne, .dst = dst };
		}

		// Branches
		constexpr static MC jmp(const Operand& dst) {
			return MC{ .op = Opcode::Jmp, .dst = dst };
		}

		constexpr static MC jz(const Operand& dst) {
			return MC{ .op = Opcode::Jz, .dst = dst };
		}

		constexpr static MC jnz(const Operand& dst) {
			return MC{ .op = Opcode::Jnz, .dst = dst };
		}

		constexpr static MC jl(const Operand& dst) {
			return MC{ .op = Opcode::Jl, .dst = dst };
		}

		constexpr static MC jle(const Operand& dst) {
			return MC{ .op = Opcode::Jle, .dst = dst };
		}

		constexpr static MC jg(const Operand& dst) {
			return MC{ .op = Opcode::Jg, .dst = dst };
		}

		constexpr static MC jge(const Operand& dst) {
			return MC{ .op = Opcode::Jge, .dst = dst };
		}

		constexpr static MC je(const Operand& dst) {
			return MC{ .op = Opcode::Je, .dst = dst };
		}

		constexpr static MC jne(const Operand& dst) {
			return MC{ .op = Opcode::Jne, .dst = dst };
		}

		// Returns
		constexpr static MC ret() {
			return MC{ .op = Opcode::Ret };
		}

		constexpr static MC ret(const Operand& src) {
			return MC{ .op = Opcode::Ret, .src = src };
		}

		constexpr static MC label(const int l) {
			return MC{ .op = Opcode::Label, .lbl = l };
		}

		constexpr static MC nop() {
			return MC{ .op = Opcode::Nop };
		}
	};

	struct FunctionMC {
		int stack_size{};
		int epi_lbl{};
		std::vector<Reg> regs_to_restore;
		std::unordered_set<Reg> claimed_callee_saved_regs;
		std::vector<MC> prologue;
		std::vector<MC> block;
		std::vector<MC> epilogue;
	};

public:
	explicit X64(IRGen& ir, X64Optimizer& optimizer);
	void module();
	void optimize(std::vector<MC>& mc);
	std::string assembly() const;

private:
	void function(const std::string& name, const CFGFunction& fn);
	void instruction(std::vector<MC>& mc, const Inst& inst);
	
	// Allocation
	// implemented in x64-allocator.cpp
	void consume(const ValueId value_id);
	TypeSize type_size(const AST::Type& type);
	void alloc_stack(const ValueId value_id, const ValueLifetime lifetime);
	void alloc_reg(const ValueId value_id, const Reg reg, const ValueLifetime lifetime);
	bool is_temporary(const ValueId value_id);
	void save_callee_reg(const Reg reg);

	// Location
	void alloc_on_demand(const ValueId value_id);
	Operand operand(const ValueId value_id);
	Operand location(const ValueId value_id);
	Operand constant(const ValueId constant_value_id);
	
	// Assembly
	std::string emit(const Operand& operand);
	void emit(std::ostream& ts, const std::vector<MC>& mc);

	IRGen& ir;
	X64Optimizer& optimizer;

	std::unordered_map<ValueId, ValueLocation> locations;
	std::unordered_map<Reg, ValueId> claimed_regs;

	// Machine code
	FunctionMC function_mc;
	//std::vector<MC> mc;
	std::stringstream function_textstream;
};
