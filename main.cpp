#include "parser.hpp"
#include "x64.hpp"
#include "x64-optimizer.hpp"
#include "bb.hpp"
#include <iostream>
#include <iomanip>
#include <format>
#include <fstream>

using namespace std;

static const map<string, Token::Type> keywords =
{
	// -- keywords --
	{"function", Token::Type::Function},
	{"return", Token::Type::Return},
	{"while", Token::Type::While},
	{"if", Token::Type::If},
	{"then", Token::Type::Then},
	{"else", Token::Type::Else},
	{"const", Token::Type::Const},
	{"var", Token::Type::Var},
	{"inline", Token::Type::Inline},
	{"do", Token::Type::Do},
	// types
	{"void", Token::Type::Void},
	{"char", Token::Type::Char},
	{"short", Token::Type::Short},
	{"int", Token::Type::Int},
	{"long", Token::Type::Long},
};

static const map<string, Token::Type> punctuation =
{
	// -- punctuation --
	{"(", Token::Type::LeftParen},
	{")", Token::Type::RightParen},
	{":", Token::Type::Colon},
	{"{", Token::Type::LeftBrace},
	{"}", Token::Type::RightBrace},
	{"[", Token::Type::LeftSqBracket},
	{"]", Token::Type::RightSqBracket},
	{"=", Token::Type::Assign},
	// logic
	{"&", Token::Type::And},
	{"|", Token::Type::Or},
	{"^", Token::Type::Xor},
	{"!", Token::Type::Not},
	// arithmetic
	{"+", Token::Type::Plus},
	{"-", Token::Type::Minus},
	{"*", Token::Type::Star},
	{"/", Token::Type::Slash},
	// comparison
	{"<",  Token::Type::Lesser},
	{">",  Token::Type::Greater},
	{"<=", Token::Type::LesserOrEqual},
	{">=", Token::Type::GreaterOrEqual},
	{"==", Token::Type::Equal},
	{"!=", Token::Type::NotEqual},
};

string tystr(const AST::Type& type) {
	string s = type.name;

	for (const auto& qual : type.qualifiers) {
		if (qual.is_const) {
			s += "const ";
		}

		if (qual.kind == AST::Type::Qualifier::Kind::Pointer) {
			s += "*";
		}

		if (qual.kind == AST::Type::Qualifier::Kind::Array) {
			s = format("[{} x {}]", s, qual.array_length);
		}

		s += ' ';
	}
	return s;
}

static std::string v(ValueId id) {
	return std::format("v{}", id);
}


void out(IRGen& irgen, const std::vector<Inst>& inst, std::ostream& os) {
	for (const auto& ins : inst) {
		// os << "; ";
		if (ins.opcode == Opcode::Label) {
			os << std::format("L{}:", ins.operands[0]) << '\n';
			continue;
		}

		if (ins.opcode == Opcode::Branch) {
			os << std::format("b v{}, L{}, L{}", ins.operands[0], ins.operands[1], ins.operands[2]) << '\n';
			continue;
		}

		if (ins.opcode == Opcode::Jump) {
			os << std::format("j L{}", ins.operands[0]) << '\n';
			continue;
		}

		// Result + type
		if (ins.result >= 0) {
			const auto& val = irgen.values.at(ins.result);
			os << std::format("{} : {} = ", v(ins.result), tystr(val.type));
		}
		// Opcode
		os << opcode_name(ins.opcode);
		// Special case: const  print literal value
		if (ins.opcode == Opcode::Const) {
			const auto& c = irgen.constants.at(ins.result);
			os << ' ';
			std::visit([&](auto&& x) {
				os << x;
			}, c.data);
		}
		// Operands
		if (!ins.operands.empty()) {
			os << ' ';
			for (size_t i = 0; i < ins.operands.size(); ++i) {
				os << v(ins.operands[i]);
				if (i + 1 < ins.operands.size()) {
					os << ", ";
				}
			}
		}
		os << '\n';
	}
}

void out(IRGen& irgen, X64& x64, std::ostream& os, bool comments = false) {
	for (const auto& [func_name, func] : irgen.mod.functions) {
		os << format("; func {}", func_name) << '\n';
		out(irgen, func.insts, os);
	}
	os << ";--------------\n";
	string line;
	while (getline(x64.function_textstream, line, '\n')) {
		os << line << '\n';
	}
}


static void bbg_out(IRGen& irgen, BasicBlockGenerator& bbg, std::ostream& os) {
	for (const auto& [fn_name, bbs] : bbg.fn_to_bbs) {
		os << "; func " << fn_name << '\n';
		for (int i = 0; i < bbs.size(); ++i) {
			os << std::format("BB{}:\n", i);
			out(irgen, bbs.at(i).inst, os);
		}
	}
}

static Parser load(const string& filename) {
	string src;
	ifstream in(filename);
	getline(in, src, '\0');
	Parser parser{ tokenize(src, keywords, punctuation) };
	return parser;
}

int main() {
	Parser parser = load("prog.in");
	auto root = parser.parse();

	if (parser.errors.size()) {
		for (const auto& err : parser.errors) {
			cout << format("error: {}", err) << '\n';
		}
	}

	IRGen irgen;
	irgen.gen(root);

	if (irgen.errors.size()) {
		for (const auto& err : irgen.errors) {
			cout << format("error: {}", err) << '\n';
		}
	}

	X64Optimizer optimizer{ irgen };
	X64 x64{ irgen, optimizer };

	x64.module();

	BasicBlockGenerator bbg{};
	bbg.module(irgen.mod);

	puts("ASSEMBLY OUTPUT IS DISABLED IN MAIN");
	bbg_out(irgen, bbg, cout);

	return 0;

	ofstream outf("prog.S");
	out(irgen, x64, outf);
	out(irgen, x64, cout, true);
	return 0;
}