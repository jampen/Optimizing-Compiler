#include "ir.hpp"

using enum Opcode;

std::vector<ValueId> Inst::read_operands() const
{
    switch (opcode)
    {
        case Store:
            return {operands[1]};

        case Add:
        case Sub:
        case Lesser:
        case LesserOrEqual:
        case Greater:
        case GreaterOrEqual:
        case Equal:
        case NotEqual:
        case And:
        case Or:
        case Xor:
            return {operands[0], operands[1]};

        case Branch:
            return {operands[0]};
        case Return:
            return {operands[0]};
    }

    return {};
}

std::vector<ValueId> Inst::written_operands() const
{
    switch (opcode)
    {
        case Const:
            return {result};
        case Store:
            return {operands[0]};

        case Add:
        case Sub:
        case Lesser:
        case LesserOrEqual:
        case Greater:
        case GreaterOrEqual:
        case Equal:
        case NotEqual:
        case And:
        case Or:
        case Xor:
            return {result};
    }

    return {};
}