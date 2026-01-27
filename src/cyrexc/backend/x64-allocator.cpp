#include "x64.hpp"

#include <format>

void X64::consume(const ValueId value_id)
{
    if (!locations.contains(value_id))
        return;
    auto& l = locations.at(value_id);
    if (l.lifetime == ValueLifetime::Persistent)
        return;
    if (l.kind == ValueLocation::Kind::Reg)
    {
        printf("consumed v%d\n", value_id);
        claimed_regs.erase(l.loc.reg);
        locations.erase(value_id);
    }
}

X64::TypeSize X64::type_size(const AST::Type& type)
{
    if (!type.qualifiers.empty())
    {
        for (const auto& qual : type.qualifiers)
            if (qual.kind == AST::Type::Qualifier::Kind::Pointer)
                return {.elem_size = RegSize::Qword, .num_bytes = 8, .is_array = false};
    }

    if (type.name == "char")
        return {.elem_size = RegSize::Byte, .num_bytes = 1, .is_array = false};
    if (type.name == "short")
        return {.elem_size = RegSize::Word, .num_bytes = 2, .is_array = false};
    if (type.name == "int")
        return {.elem_size = RegSize::Dword, .num_bytes = 4, .is_array = false};
    if (type.name == "long")
        return {.elem_size = RegSize::Qword, .num_bytes = 8, .is_array = false};
    return {.elem_size = RegSize::Byte, .num_bytes = 1, .is_array = false};
}

void X64::alloc_stack(const ValueId value_id, const ValueLifetime lifetime)
{
    const auto& value = ir.get_value_by_id(value_id);
    function_mc.stack_size += type_size(value.type).num_bytes;
    locations[value_id] = {.kind = ValueLocation::Kind::Stack, .loc = function_mc.stack_size, .lifetime = lifetime};
}

void X64::alloc_reg(const ValueId value_id, const Reg reg, const ValueLifetime lifetime)
{
    if (claimed_regs.contains(reg))
    {
        throw std::runtime_error(std::format("internal error: {} is already claimed", reg_to_string(reg)));
    }
    claimed_regs[reg] = value_id;
    locations[value_id] = {.kind = ValueLocation::Kind::Reg, .loc = (int)reg, .lifetime = lifetime};
}

bool X64::is_temporary(const ValueId value_id)
{
    if (!locations.contains(value_id))
    {
        return false;
    }
    return locations.at(value_id).lifetime == ValueLifetime::Temporary;
}

void X64::save_callee_reg(const Reg reg)
{
    if (!is_reg_callee_saved(reg))
    {
        throw std::runtime_error(std::format("internal error: {} is not a callee saved reg", reg_to_string(reg)));
    }
    if (function_mc.claimed_callee_saved_regs.contains(reg))
        return;
    function_mc.claimed_callee_saved_regs.emplace(reg);
    function_mc.regs_to_restore.push_back(reg);
}

void X64::alloc_on_demand(const ValueId value_id)
{
    // Is it already allocated?
    if (locations.contains(value_id))
        return;

    // First, try the volatile regs
    for (auto r : volatile_regs)
    {
        if (r == Reg::rax)
            continue;
        if (!claimed_regs.contains(r))
        {
            alloc_reg(value_id, r, ValueLifetime::Temporary);
            return;
        }
    }

    // Then the callee saved
    for (auto r : callee_saved_regs)
    {
        if (!claimed_regs.contains(r))
        {
            alloc_reg(value_id, r, ValueLifetime::Temporary);
            save_callee_reg(r);
            return;
        }
    }

    // Spill
    alloc_stack(value_id, ValueLifetime::Temporary);
}