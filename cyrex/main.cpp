#include "frontend/parser.hpp"
#include "frontend/semantics.hpp"

#include "backend/x64.hpp"
#include "backend/x64-optimizer.hpp"

#include <iostream>
#include <iomanip>
#include <format>
#include <fstream>

#include <argparse/argparse.hpp>

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

static void print_ir_instruction(ostream& outfile, const Inst& ins, const IRGen& irgen) {
	//outfile << "; ";
	if (ins.opcode == Opcode::Label) {
		outfile << format("L{}:", ins.operands[0]) << '\n';
		return;
	}

	if (ins.opcode == Opcode::Branch) {
		outfile << format("b v{}, L{}, L{}", ins.operands[0], ins.operands[1], ins.operands[2]) << '\n';
		return;
	}

	if (ins.opcode == Opcode::Jump) {
		outfile << format("j L{}", ins.operands[0]) << '\n';
		return;
	}

	// Result + type
	if (ins.result >= 0) {
		const auto& val = irgen.get_value_by_id(ins.result);
		outfile << format("{} : {} = ", v(ins.result), tystr(val.type));
	}
	// Opcode
	outfile << opcode_name(ins.opcode);
	// Special case: const  print literal value
	if (ins.opcode == Opcode::Const) {
		const auto& c = irgen.get_literal_by_id(ins.result);
		outfile << ' ';
		visit([&](auto&& x) {
			outfile << x;
		}, c.data);
	}
	// Operands
	if (!ins.operands.empty()) {
		outfile << ' ';
		for (size_t i = 0; i < ins.operands.size(); ++i) {
			outfile << v(ins.operands[i]);
			if (i + 1 < ins.operands.size()) {
				outfile << ", ";
			}
		}
	}
	outfile << '\n';
}

static void print_ir(ostream& os, const IRGen& irgen) {
	for (const auto& [cfg_name, cfg] : irgen.get_functions()) {
		for (const auto& blk : cfg.blocks) {
			os << format("BB{}:\n", blk.lbl_entry);
			for (const auto& ins : blk.inst) {
				print_ir_instruction(os, ins, irgen);
			}
		}
	}
}

static void write_ir(const string& filename, const IRGen& irgen) {
	ofstream outfile(filename);
	if (!outfile) {
		throw runtime_error(format("error creating outfile: {}", filename));
	}
	print_ir(outfile, irgen);
}

static void write_assembly(const string& filename, const X64& x64) {
	ofstream outfile(filename);
	if (!outfile) {
		throw runtime_error(format("error creating outfile: {}", filename));
	}

	outfile << x64.assembly() << '\n';
}

int main(int argc, const char* argv[]) {
	// prepare arguments
	argparse::ArgumentParser program("cyrexc");
	program.add_argument("input_files")
		.help("a list of files to compile")
		.nargs(1, std::numeric_limits<size_t>::max())
		.required();

	program.add_argument("--ir")
		.help("output immediate representation")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("--optimized")
		.help("compile with optimization")
		.default_value(false)
		.implicit_value(true);

	try {
		program.parse_args(argc, argv);
	} catch (const exception& err) {
		cerr << "fatal: " << err.what() << '\n';
		cerr << program << '\n';
		return EXIT_FAILURE;
	}

	const bool output_ir = program.get<bool>("--ir");
	const bool is_optimized = program.get<bool>("--optimized");
	const auto files = program.get<std::vector<std::string>>("input_files");

	for (const auto& filename : files) {
		if (!filename.ends_with(".cyrex")) {
			throw runtime_error(format("source file {} must end in .cyrex", filename));
		}

		vector<Token> tokens;
		try {
			tokens = tokenize(read_file(filename), keywords, punctuation);
		} catch (const runtime_error& rte) {
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

		string assembly_filename = filename.substr(0, filename.size() - 6) + ".S";
		string ir_filename = filename.substr(0, filename.size() - 6) + ".ir";

		X64Optimizer optimizer{ irgen };
		optimizer.is_enabled = is_optimized;

		X64 x64(irgen, optimizer);
		x64.module();

		write_assembly(assembly_filename, x64);
		if (output_ir) write_ir(ir_filename, irgen);
	}

	return 0;
}