#pragma once
#include "../data/CandleData.h"
#include "EMA.h"
#include "RSI.h"
#include "ATR.h"
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <memory>

namespace crypto {

// ── Token types for Pine Script v6 lexer ─────────────────────────────────────
enum class PineTok {
    NUMBER, STRING, IDENT,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NEQ, LT, GT, LTE, GTE,
    ASSIGN, REASSIGN,
    LPAREN, RPAREN, LBRACKET, RBRACKET,
    COMMA, DOT, COLON, QUESTION,
    KW_AND, KW_OR, KW_NOT,
    KW_TRUE, KW_FALSE, KW_NA,
    KW_VAR, KW_IF, KW_ELSE,
    NEWLINE, END_OF_FILE
};

struct PineToken {
    PineTok type;
    std::string value;
    int line{0};
};

// ── Expression AST ───────────────────────────────────────────────────────────
struct PineExpr {
    enum class Kind {
        LITERAL_NUM, LITERAL_STR, LITERAL_BOOL, LITERAL_NA,
        VARIABLE, SERIES_ACCESS,
        BINARY_OP, UNARY_OP, TERNARY,
        FUNC_CALL
    };
    Kind kind;
    double numVal{0.0};
    bool boolVal{false};
    std::string strVal;
    std::string op;
    std::vector<std::shared_ptr<PineExpr>> children;
    std::map<std::string, std::shared_ptr<PineExpr>> namedArgs;
};

// ── Statement ────────────────────────────────────────────────────────────────
struct PineStmt {
    enum class Kind { INDICATOR, ASSIGN, VAR_DECL, PLOT, EXPR };
    Kind kind;
    std::string name;
    std::shared_ptr<PineExpr> expr;
    std::string plotTitle;
};

// ── Parsed script ────────────────────────────────────────────────────────────
struct PineScript {
    std::string name;
    bool overlay{false};
    int version{6};
    std::vector<PineStmt> statements;
};

// ── Runtime interpreter ──────────────────────────────────────────────────────
class PineRuntime {
public:
    void load(const PineScript& script);
    std::map<std::string, double> update(const Candle& c);
    const std::string& name() const { return script_.name; }
    bool loaded() const { return !script_.name.empty(); }

private:
    PineScript script_;
    int barIndex_{0};
    Candle cur_;
    std::deque<Candle> history_;

    std::map<std::string, std::deque<double>> series_;
    std::map<std::string, double> vars_;
    std::map<std::string, std::unique_ptr<EMA>> emas_;
    std::map<std::string, std::unique_ptr<RSI>> rsis_;
    std::map<std::string, std::unique_ptr<ATR>> atrs_;
    std::map<std::string, double> prevVals_;
    std::map<std::string, double> plots_;

    double eval(const std::shared_ptr<PineExpr>& e);
    void exec(const PineStmt& s);
    double callFunc(const std::string& fullName,
                    const std::vector<double>& args);
    double seriesAt(const std::string& name, int offset) const;
    void store(const std::string& name, double v);
};

// ── Converter / parser ───────────────────────────────────────────────────────
class PineConverter {
public:
    static PineScript parseFile(const std::string& filepath);
    static PineScript parseSource(const std::string& source);
    static std::string generateCpp(const PineScript& script,
                                   const std::string& className);

private:
    static std::vector<PineToken> tokenize(const std::string& src);

    class Parser {
    public:
        explicit Parser(std::vector<PineToken> tokens);
        PineScript parse();
    private:
        std::vector<PineToken> tok_;
        size_t pos_{0};

        const PineToken& cur() const;
        PineToken advance();
        bool match(PineTok t);
        bool check(PineTok t) const;
        void expect(PineTok t);
        void skipNL();

        PineStmt parseStmt();
        std::shared_ptr<PineExpr> parseExpr();
        std::shared_ptr<PineExpr> parseTernary();
        std::shared_ptr<PineExpr> parseOr();
        std::shared_ptr<PineExpr> parseAnd();
        std::shared_ptr<PineExpr> parseCmp();
        std::shared_ptr<PineExpr> parseAdd();
        std::shared_ptr<PineExpr> parseMul();
        std::shared_ptr<PineExpr> parseUnary();
        std::shared_ptr<PineExpr> parsePrimary();
        std::shared_ptr<PineExpr> parseCall(const std::string& name);
    };
};

} // namespace crypto
