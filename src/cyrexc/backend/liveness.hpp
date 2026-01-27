#pragma once
#include "irgen.hpp"

class LivenessAnalysis
{
public:
    explicit LivenessAnalysis(IRGen& ir);
    void analyze();

private:
    void function(CFGFunction& function);
    void analyze_fn_liveness(CFGFunction& function);
    void analyze_inst_liveness(BasicBlock& blk);

private:
    IRGen& ir;
};