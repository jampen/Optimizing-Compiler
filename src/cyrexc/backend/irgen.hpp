#pragma once
#include "ir.hpp"

#include <optional>

// TODO Make class / standalone function
struct BasicBlockGenerator
{
    std::vector<BasicBlock> function(const LinearFunction& fn);
    void link_blocks(std::vector<BasicBlock>& blocks, const LinearFunction& fn);
    const std::vector<BasicBlock>& get_fn_bbs(const std::string& function_name) const;
    std::unordered_map<std::string, std::vector<BasicBlock>> fn_to_bbs;
};


class IRGen
{
private:
    // TODO remove
    struct Scope
    {
        std::unordered_map<std::string, ValueId> symbols;
    };

public:
    ValueId gen(const AST::Ptr& ptr);
    const Value& get_value_by_id(const ValueId value_id) const;
    const Literal& get_literal_by_id(const ValueId value_id) const;
    const CFGFunction& get_function_by_name(const std::string& name) const;
    CFGFunction& get_function_by_name(const std::string& name);

    constexpr const auto& get_functions() const
    {
        return mod.functions;
    }
    constexpr auto& get_functions()
    {
        return mod.functions;
    }
    bool literal_exists(const ValueId value_id) const;

public:
    constexpr bool has_errors() const
    {
        return !errors.empty();
    }
    constexpr const auto& get_errors() const
    {
        return errors;
    }

private:
    // dispatchers
    ValueId top(const AST::Top& top);
    ValueId stmt(const AST::Stmt& stmt);
    ValueId expr(const AST::Expr& expr);

private:
    // top
    ValueId root(const AST::Root& root);
    ValueId function(const AST::Function& function);
    ValueId parameter_list(const AST::ParameterList& parameter_list);

private:
    // statements
    ValueId if_stmt(const AST::IfStmt& if_stmt);
    ValueId while_stmt(const AST::WhileStmt& while_stmt);
    ValueId block_stmt(const AST::BlockStmt& block);
    ValueId return_stmt(const AST::ReturnStmt& ret);
    ValueId var_stmt(const AST::VariableStmt& var);

private:
    // expressions
    ValueId if_expr(const AST::IfExpr& if_expr);
    ValueId while_expr(const AST::WhileExpr& while_expr);
    ValueId literal_expr(const AST::LiteralExpr& literal);
    ValueId identifier_expr(const AST::IdentifierExpr& identifier);
    ValueId binary_expr(const AST::BinaryExpr& binary);
    ValueId assign_expr(const AST::AssignExpr& assign);

    ValueId tuple_expr(const AST::TupleExpr& tup_expr);
    ValueId tuple_assign_expr(const AST::TupleAssignExpr& tup_assign_expr);

private:
    Literal parse_literal(const AST::LiteralExpr literal);

private:
    // errors
    void push_error(const std::string& error_message);

private:
    // insts
    void push_inst(const Opcode o, const ValueId result, const std::vector<ValueId>& operands = {});
    void push_label(const LabelId label_id);

private:
    // produces a new value and returns its id
    ValueId new_value(const AST::Type& type);
    ValueId new_tuple();
    void push_tuple_elem(const ValueId tuple_id, const ValueId value_id);

private:
    void enter_scope();
    std::optional<ValueId> find_symbol(const std::string& name);
    void exit_scope();

private:
    LabelId new_label();

private:
    std::vector<std::string> errors;
    std::vector<Value> values;
    std::unordered_map<ValueId, Literal> literals;
    std::unordered_map<ValueId, Tuple> tuples;
    std::unordered_map<std::string, LinearFunction> functions;

    std::vector<Scope> scopes;
    LabelId next_label_id{};
    Module mod;
    LinearFunction* current_fn{};
    BasicBlockGenerator bbg;
};