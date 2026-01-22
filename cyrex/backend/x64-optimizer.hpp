#pragma once
#include "x64.hpp"

struct X64Optimizer {
	using MC = X64::MC;
	using Operand = X64::Operand;
	using Reg = X64::Reg;
	IRGen& ir;

	bool pass(std::vector<MC>& mc);
	bool pass_peephole(std::vector<MC>& mc);
	bool pass_unused_labels(std::vector<MC>& mc);
	void remove_redundant_push_pop(std::vector<MC>& mc);
};