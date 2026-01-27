#include "liveness.hpp"

LivenessAnalysis::LivenessAnalysis(IRGen& ir) : ir(ir)
{
}

void LivenessAnalysis::analyze()
{
    for (auto& [cfg_name, cfg] : ir.get_functions())
    {
        function(cfg);
    }
}

void LivenessAnalysis::function(CFGFunction& function)
{
    // step 1: detect used vars
    analyze_fn_liveness(function);
    for (BasicBlock& blk : function.blocks)
    {
        analyze_inst_liveness(blk);
    }
}

void LivenessAnalysis::analyze_fn_liveness(CFGFunction& function)
{
    for (auto& blk : function.blocks)
    {
        // make emtpy
        blk.in = {};
        blk.out = {};
        blk.def = {};
        blk.use = {};

        for (Inst& ins : blk.inst)
        {
            for (ValueId read : ins.read_operands())
            {
                if (!blk.def.contains(read))
                {
                    blk.use.insert(read);
                }
            }

            for (ValueId written : ins.written_operands())
            {
                blk.def.insert(written);
            }
        }
    }

    // step 2: find in and out
    bool changed{};
    do
    {
        changed = false;

        for (auto it = function.blocks.rbegin(); it != function.blocks.rend(); it = std::next(it))
        {
            auto& blk = *it;
            std::unordered_set<ValueId> new_out{};
            for (BasicBlock* succ : blk.successors)
            {
                new_out.insert(succ->in.begin(), succ->in.end());
            }

            std::unordered_set<ValueId> new_in{};
            for (const ValueId value : new_out)
            {
                if (!blk.def.contains(value))
                {
                    new_in.insert(value);
                }
            }

            // anything changed?
            if (new_in != blk.in || new_out != blk.out)
            {
                blk.in = std::move(new_in);
                blk.out = std::move(new_out);
                changed = true;
            }
        }
    } while (changed);
}

void LivenessAnalysis::analyze_inst_liveness(BasicBlock& blk)
{
    std::unordered_set<ValueId> live = blk.out;

    for (auto it = blk.inst.rbegin(); it != blk.inst.rend(); it = std::next(it))
    {
        Inst& ins = *it;

        // live after this instruction
        ins.live_out = live;

        // remove variables overwritten
        for (const ValueId v : ins.written_operands())
        {
            live.erase(v);
        }

        // add variables read
        for (const ValueId v : ins.read_operands())
        {
            live.insert(v);
        }

        ins.live_out = live;
    }
}
