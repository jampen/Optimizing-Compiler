#include "x64.hpp"
#include <format>

void X64::produces(const ValueId value_id, const ValueLifetime lifetime) {
	if (locations.contains(value_id)) return;

	if (lifetime == ValueLifetime::Scratch) {
		// scratch value are produced on demand
		// alloc_reg(value_id, Reg::rax, lifetime);
		return;
	}

	// prefer to use volatile registers
	for (const auto vol_reg : volatile_regs) {
		// rax is totally scratch and should never be used as a variable
		if (vol_reg == Reg::rax) continue;
		if (claimed_regs.contains(vol_reg)) continue;
		alloc_reg(value_id, vol_reg, lifetime);
		return;
	}

	// otherwise use callee saved
	for (const auto cs_reg : callee_saved_regs) {
		if (claimed_regs.contains(cs_reg)) continue;
		alloc_reg(value_id, cs_reg, lifetime);
		return;
	}

	// if we fail to allocate a reg, spill to the stack
	alloc_stack(value_id, lifetime);
}

void X64::consume(const ValueId value_id) {
	if (!locations.contains(value_id)) return;
	auto& l = locations.at(value_id);
	if (l.lifetime == ValueLifetime::Persistent) return;

	if (l.kind == ValueLocation::Kind::Reg) {
		claimed_regs.erase(l.loc.reg);
		locations.erase(value_id);
	}
}

X64::TypeSize X64::type_size(const AST::Type& type) {
	if (!type.qualifiers.empty()) {
		for (const auto& qual : type.qualifiers)
			if (qual.kind == AST::Type::Qualifier::Kind::Pointer)
				return { .elem_size = RegSize::Qword, .num_bytes = 8, .is_array = false };
	}

	if (type.name == "char")  return { .elem_size = RegSize::Byte, .num_bytes = 1, .is_array = false };
	if (type.name == "short") return { .elem_size = RegSize::Word, .num_bytes = 2, .is_array = false };
	if (type.name == "int")   return { .elem_size = RegSize::Dword, .num_bytes = 4, .is_array = false };
	if (type.name == "long")  return { .elem_size = RegSize::Qword, .num_bytes = 8, .is_array = false };
	return { .elem_size = RegSize::Byte, .num_bytes = 1, .is_array = false };
}

void X64::alloc_stack(const ValueId value_id, const ValueLifetime lifetime) {
	auto& value = ir.values[value_id];
	stack_size += type_size(value.type).num_bytes;
	locations[value_id] = {
		.kind = ValueLocation::Kind::Stack,
		.loc = stack_size,
		.lifetime = lifetime };
}

void X64::alloc_reg(const ValueId value_id, const Reg reg, const ValueLifetime lifetime) {
	if (claimed_regs.contains(reg)) {
		throw std::runtime_error(std::format("internal error: {} is already claimed", reg_to_string(reg)));
	}
	claimed_regs[reg] = value_id;
	locations[value_id] = {
		.kind = ValueLocation::Kind::Reg,
		.loc = (int)reg,
		.lifetime = lifetime
	};
}

bool X64::is_temporary(const ValueId value_id) {
	if (!locations.contains(value_id)) {
		return false;
	}
	return locations.at(value_id).lifetime == ValueLifetime::Temporary;
}

void X64::alloc_on_demand(const ValueId value_id) {
	// First, try the volatile regs
	for (auto r : volatile_regs) {
		if (r == Reg::rax) continue;
		if (!claimed_regs.contains(r)) {
			alloc_reg(value_id, r, ValueLifetime::Temporary);
			return;
		}
	}

	// Then the callee saved
	for (auto r : callee_saved_regs) {
		if (!claimed_regs.contains(r)) {
			alloc_reg(value_id, r, ValueLifetime::Temporary);
			return;
		}
	}

	// Spill
	alloc_stack(value_id, ValueLifetime::Temporary);
}