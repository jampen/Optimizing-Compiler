#include "frontend/parser.hpp"
#include "frontend/semantics.hpp"

#include "backend/x64.hpp"
#include "backend/x64-optimizer.hpp"

#include <iostream>
#include <iomanip>
#include <format>
#include <fstream>

using namespace std;


// TODO constexpr maps

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
	{"[", Token::Type::LeftSqBracket},
	{"]", Token::Type::RightSqBracket},
	{":", Token::Type::Colon},
	{"{", Token::Type::LeftBrace},
	{"}", Token::Type::RightBrace},
	{"=", Token::Type::Assign},
	{",", Token::Type::Comma},
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

static string tystr(const AST::Type& type) {
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

static string v(ValueId id) {
	return format("v{}", id);
}


static void out(IRGen& irgen, const vector<Inst>& inst, ostream& os) {
	for (const auto& ins : inst) {
		//os << "; ";
		if (ins.opcode == Opcode::Label) {
			os << format("L{}:", ins.operands[0]) << '\n';
			continue;
		}

		if (ins.opcode == Opcode::Branch) {
			os << format("b v{}, L{}, L{}", ins.operands[0], ins.operands[1], ins.operands[2]) << '\n';
			continue;
		}

		if (ins.opcode == Opcode::Jump) {
			os << format("j L{}", ins.operands[0]) << '\n';
			continue;
		}

		// Result + type
		if (ins.result >= 0) {
			const auto& val = irgen.get_value_by_id(ins.result);
			os << format("{} : {} = ", v(ins.result), tystr(val.type));
		}
		// Opcode
		os << opcode_name(ins.opcode);
		// Special case: const  print literal value
		if (ins.opcode == Opcode::Const) {
			const auto& c = irgen.get_literal_by_id(ins.result);
			os << ' ';
			visit([&](auto&& x) {
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

static string read_file(const string& filename) {
	ifstream infile(filename);
	if (!infile) {
		throw runtime_error(format("file not found: {}", filename));
	} 
	string src;
	ifstream in(filename);
	getline(in, src, '\0');
	return src;
}

int main() {
	vector<Token> tokens;
	try {
		tokens = tokenize(read_file("prog.in"), keywords, punctuation);
	}
	catch (const runtime_error& rte) {
		cerr << rte.what() << '\n';
		return EXIT_FAILURE;
	}

	Parser parser;
	auto root = parser.parse(tokens);

	if (parser.has_errors()) {
		for (const auto& err : parser.get_errors()) {
			cout << format("error: {}", err.message) << '\n';
		}
		return EXIT_FAILURE;
	}

	SemanticAnalyzer sa;
	sa.analyze(root);

	IRGen irgen;
	irgen.gen(root);

	if (irgen.has_errors()) {
		for (const auto& err : irgen.get_errors()) {
			cout << format("error: {}", err) << '\n';
		}
		return EXIT_FAILURE;
	}
	
	ofstream outf("prog.S");
	X64Optimizer optimizer{ irgen };	

	X64 x64 (irgen, optimizer);
	x64.module();
	outf << x64.assembly();
	return 0;
}