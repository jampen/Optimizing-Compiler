#pragma once
#include "ir.hpp"

struct BasicBlock {
	LabelId lbl_entry;
	std::vector<Inst> inst;
	std::vector<BasicBlock*> successors;
};


struct BasicBlockGenerator {
	void module(const Module& mod);
	std::vector<BasicBlock> function(const Function& fn);
	void link_blocks(std::vector<BasicBlock>& blocks, const Function& fn);
	const std::vector<BasicBlock>& get_fn_bbs(const std::string& function_name) const;
	std::unordered_map<std::string, std::vector<BasicBlock>> fn_to_bbs;
};