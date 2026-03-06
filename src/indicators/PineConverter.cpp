#include "PineConverter.h"
#include "PineScriptIndicator.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <regex>

namespace crypto {

// ═════════════════════════════════════════════════════════════════════════════
//  Tokenizer
// ═════════════════════════════════════════════════════════════════════════════
std::vector<PineToken> PineConverter::tokenize(const std::string& src) {
    std::vector<PineToken> tokens;
    int line = 1;
    size_t i = 0;
    auto push = [&](PineTok t, const std::string& v) {
        tokens.push_back({t, v, line});
    };

    while (i < src.size()) {
        char ch = src[i];

        // Newline
        if (ch == '\n') { push(PineTok::NEWLINE, "\\n"); ++line; ++i; continue; }
        if (ch == '\r') { ++i; continue; }

        // Whitespace
        if (ch == ' ' || ch == '\t') { ++i; continue; }

        // Comments
        if (ch == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            // Check for version annotation: //@version=N
            while (i < src.size() && src[i] != '\n') ++i;
            // Comments are skipped; version is detected in parser via regex on source
            continue;
        }

        // Strings
        if (ch == '"' || ch == '\'') {
            char q = ch; ++i;
            std::string s;
            while (i < src.size() && src[i] != q) s += src[i++];
            if (i < src.size()) ++i;
            push(PineTok::STRING, s);
            continue;
        }

        // Numbers
        if (std::isdigit(ch) || (ch == '.' && i + 1 < src.size() && std::isdigit(src[i + 1]))) {
            std::string n;
            while (i < src.size() && (std::isdigit(src[i]) || src[i] == '.'))
                n += src[i++];
            push(PineTok::NUMBER, n);
            continue;
        }

        // Identifiers / keywords
        if (std::isalpha(ch) || ch == '_') {
            std::string id;
            while (i < src.size() && (std::isalnum(src[i]) || src[i] == '_'))
                id += src[i++];
            // Keywords
            if (id == "and")   { push(PineTok::KW_AND, id); continue; }
            if (id == "or")    { push(PineTok::KW_OR, id); continue; }
            if (id == "not")   { push(PineTok::KW_NOT, id); continue; }
            if (id == "true")  { push(PineTok::KW_TRUE, id); continue; }
            if (id == "false") { push(PineTok::KW_FALSE, id); continue; }
            if (id == "na")    { push(PineTok::KW_NA, id); continue; }
            if (id == "var")   { push(PineTok::KW_VAR, id); continue; }
            if (id == "if")    { push(PineTok::KW_IF, id); continue; }
            if (id == "else")  { push(PineTok::KW_ELSE, id); continue; }
            push(PineTok::IDENT, id);
            continue;
        }

        // Two-char operators
        if (i + 1 < src.size()) {
            std::string two{ch, src[i + 1]};
            if (two == ":=") { push(PineTok::REASSIGN, two); i += 2; continue; }
            if (two == "==") { push(PineTok::EQ,  two); i += 2; continue; }
            if (two == "!=") { push(PineTok::NEQ, two); i += 2; continue; }
            if (two == "<=") { push(PineTok::LTE, two); i += 2; continue; }
            if (two == ">=") { push(PineTok::GTE, two); i += 2; continue; }
        }

        // Single-char operators
        switch (ch) {
            case '+': push(PineTok::PLUS,     "+"); break;
            case '-': push(PineTok::MINUS,    "-"); break;
            case '*': push(PineTok::STAR,     "*"); break;
            case '/': push(PineTok::SLASH,    "/"); break;
            case '%': push(PineTok::PERCENT,  "%"); break;
            case '<': push(PineTok::LT,       "<"); break;
            case '>': push(PineTok::GT,       ">"); break;
            case '=': push(PineTok::ASSIGN,   "="); break;
            case '(': push(PineTok::LPAREN,   "("); break;
            case ')': push(PineTok::RPAREN,   ")"); break;
            case '[': push(PineTok::LBRACKET, "["); break;
            case ']': push(PineTok::RBRACKET, "]"); break;
            case ',': push(PineTok::COMMA,    ","); break;
            case '.': push(PineTok::DOT,      "."); break;
            case ':': push(PineTok::COLON,    ":"); break;
            case '?': push(PineTok::QUESTION, "?"); break;
            default: break;
        }
        ++i;
    }
    push(PineTok::END_OF_FILE, "");
    return tokens;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Parser
// ═════════════════════════════════════════════════════════════════════════════
PineConverter::Parser::Parser(std::vector<PineToken> tokens)
    : tok_(std::move(tokens)) {}

const PineToken& PineConverter::Parser::cur() const { return tok_[pos_]; }

PineToken PineConverter::Parser::advance() {
    auto t = tok_[pos_];
    if (pos_ < tok_.size() - 1) ++pos_;
    return t;
}

bool PineConverter::Parser::match(PineTok t) {
    if (check(t)) { advance(); return true; }
    return false;
}

bool PineConverter::Parser::check(PineTok t) const { return cur().type == t; }

void PineConverter::Parser::expect(PineTok t) {
    if (!match(t))
        throw std::runtime_error("Pine parse error at line " +
            std::to_string(cur().line) + ": unexpected '" + cur().value + "'");
}

void PineConverter::Parser::skipNL() {
    while (check(PineTok::NEWLINE)) advance();
}

PineScript PineConverter::Parser::parse() {
    PineScript script;
    script.name = "user_indicator";
    skipNL();
    while (!check(PineTok::END_OF_FILE)) {
        size_t savedPos = pos_;
        try {
            auto stmt = parseStmt();
            if (stmt.kind == PineStmt::Kind::INDICATOR) {
                script.name = stmt.name;
                if (stmt.expr && stmt.expr->boolVal) script.overlay = true;
            }
            script.statements.push_back(std::move(stmt));
        } catch (...) {
            // Skip to next line on parse error (resilient parsing)
            pos_ = savedPos;
            while (!check(PineTok::NEWLINE) && !check(PineTok::END_OF_FILE))
                advance();
        }
        skipNL();
    }
    return script;
}

PineStmt PineConverter::Parser::parseStmt() {
    PineStmt s;

    // var declaration
    if (check(PineTok::KW_VAR)) {
        advance();
        // Optional type annotation after 'var': var int x = 0, var linefill x = na, etc.
        if (check(PineTok::IDENT) && (cur().value == "int" || cur().value == "float" ||
            cur().value == "string" || cur().value == "bool" ||
            cur().value == "linefill" || cur().value == "label" ||
            cur().value == "line" || cur().value == "box" || cur().value == "table" ||
            cur().value == "color" || cur().value == "array")) {
            advance(); // skip the type
        }
        std::string name = cur().value;
        expect(PineTok::IDENT);
        expect(PineTok::ASSIGN);
        s.kind = PineStmt::Kind::VAR_DECL;
        s.name = name;
        s.expr = parseExpr();
        return s;
    }

    // plot(...)
    if (check(PineTok::IDENT) && cur().value == "plot") {
        advance();
        expect(PineTok::LPAREN);
        s.kind = PineStmt::Kind::PLOT;
        s.expr = parseExpr();
        s.plotTitle = "plot";
        if (match(PineTok::COMMA)) {
            // Try to read title (could be string or named arg)
            if (check(PineTok::STRING)) {
                s.plotTitle = cur().value;
                advance();
            } else if (check(PineTok::IDENT) && cur().value == "title") {
                advance(); expect(PineTok::ASSIGN);
                if (check(PineTok::STRING)) {
                    s.plotTitle = cur().value;
                    advance();
                }
            }
            // Skip remaining args
            while (!check(PineTok::RPAREN) && !check(PineTok::END_OF_FILE)) advance();
        }
        expect(PineTok::RPAREN);
        return s;
    }

    // Visual statements: plotshape/plotchar/plotarrow/bgcolor/barcolor/fill/hline/alertcondition
    // Skip arguments and treat as expression statement
    if (check(PineTok::IDENT) && (
            cur().value == "plotshape" || cur().value == "plotchar" ||
            cur().value == "plotarrow" || cur().value == "bgcolor"  ||
            cur().value == "barcolor"  || cur().value == "fill"     ||
            cur().value == "hline"     || cur().value == "alertcondition")) {
        advance();
        if (check(PineTok::LPAREN)) {
            advance(); // skip (
            int depth = 1;
            while (depth > 0 && !check(PineTok::END_OF_FILE)) {
                if (check(PineTok::LPAREN)) depth++;
                if (check(PineTok::RPAREN)) depth--;
                if (depth > 0) advance();
            }
            if (check(PineTok::RPAREN)) advance();
        }
        s.kind = PineStmt::Kind::EXPR;
        s.expr = std::make_shared<PineExpr>();
        s.expr->kind = PineExpr::Kind::LITERAL_NUM;
        s.expr->numVal = 0.0;
        return s;
    }

    // indicator(...) or strategy(...) — but NOT strategy.entry/exit/close
    if (check(PineTok::IDENT) && (cur().value == "indicator" || cur().value == "strategy")) {
        size_t save = pos_;
        std::string kw = cur().value;
        advance();
        if (check(PineTok::LPAREN)) {
            advance(); // consume LPAREN
            s.kind = PineStmt::Kind::INDICATOR;
            if (check(PineTok::STRING)) {
                s.name = cur().value;
                advance();
            }
            // Parse optional named args (overlay=true)
            auto overlayExpr = std::make_shared<PineExpr>();
            overlayExpr->kind = PineExpr::Kind::LITERAL_BOOL;
            overlayExpr->boolVal = false;
            while (!check(PineTok::RPAREN) && !check(PineTok::END_OF_FILE)) {
                if (check(PineTok::IDENT) && cur().value == "overlay") {
                    advance(); expect(PineTok::ASSIGN);
                    if (check(PineTok::KW_TRUE)) { overlayExpr->boolVal = true; advance(); }
                    else if (check(PineTok::KW_FALSE)) { advance(); }
                } else {
                    advance();
                }
            }
            s.expr = overlayExpr;
            expect(PineTok::RPAREN);
            return s;
        }
        // Not a declaration (e.g., strategy.entry) — rewind
        pos_ = save;
    }

    // 'type' declaration block — skip to end of block
    if (check(PineTok::IDENT) && cur().value == "type") {
        while (!check(PineTok::NEWLINE) && !check(PineTok::END_OF_FILE)) advance();
        s.kind = PineStmt::Kind::EXPR;
        s.expr = std::make_shared<PineExpr>();
        s.expr->kind = PineExpr::Kind::LITERAL_NUM;
        s.expr->numVal = 0.0;
        return s;
    }

    // Typed variable declaration: int/float/string/bool IDENT = expr
    if (check(PineTok::IDENT) && (cur().value == "int" || cur().value == "float" ||
        cur().value == "string" || cur().value == "bool")) {
        size_t save = pos_;
        advance(); // skip type
        if (check(PineTok::IDENT)) {
            std::string name = cur().value;
            advance();
            if (match(PineTok::ASSIGN) || match(PineTok::REASSIGN)) {
                s.kind = PineStmt::Kind::ASSIGN;
                s.name = name;
                s.expr = parseExpr();
                return s;
            }
        }
        pos_ = save;
    }

    // Assignment: ident = expr  OR  ident := expr
    // Also handle function definition: ident(params) => expr (skip it)
    if (check(PineTok::IDENT)) {
        size_t save = pos_;
        std::string name = cur().value;
        advance();

        // Function definition with =>: name(params) => expr
        if (check(PineTok::LPAREN)) {
            advance(); // skip (
            // Scan forward to see if there's '=>' after closing ')'
            int depth = 1;
            bool foundArrow = false;
            while (!check(PineTok::END_OF_FILE) && !check(PineTok::NEWLINE)) {
                if (check(PineTok::LPAREN)) depth++;
                if (check(PineTok::RPAREN)) { depth--; if (depth == 0) { advance(); break; } }
                advance();
            }
            // Check for '=>' pattern (ASSIGN followed by GT)
            if (check(PineTok::ASSIGN) && pos_ + 1 < tok_.size() && tok_[pos_ + 1].type == PineTok::GT) {
                foundArrow = true;
            }
            if (foundArrow) {
                // Skip the rest of the function definition line
                while (!check(PineTok::NEWLINE) && !check(PineTok::END_OF_FILE)) advance();
                s.kind = PineStmt::Kind::EXPR;
                s.expr = std::make_shared<PineExpr>();
                s.expr->kind = PineExpr::Kind::LITERAL_NUM;
                s.expr->numVal = 0.0;
                return s;
            }
            pos_ = save; // rewind
        }

        // Regular assignment
        if (pos_ == save + 1 && (match(PineTok::ASSIGN) || match(PineTok::REASSIGN))) {
            s.kind = PineStmt::Kind::ASSIGN;
            s.name = name;
            s.expr = parseExpr();
            return s;
        }
        // Not an assignment, rewind
        pos_ = save;
    }

    // Expression statement
    s.kind = PineStmt::Kind::EXPR;
    s.expr = parseExpr();
    return s;
}

// ── Expression parsing (recursive descent) ───────────────────────────────────

std::shared_ptr<PineExpr> PineConverter::Parser::parseExpr() { return parseTernary(); }

std::shared_ptr<PineExpr> PineConverter::Parser::parseTernary() {
    auto e = parseOr();
    if (match(PineTok::QUESTION)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::TERNARY;
        node->children.push_back(e);
        node->children.push_back(parseOr());
        expect(PineTok::COLON);
        node->children.push_back(parseOr());
        return node;
    }
    return e;
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseOr() {
    auto left = parseAnd();
    while (match(PineTok::KW_OR)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::BINARY_OP;
        node->op = "or";
        node->children = {left, parseAnd()};
        left = node;
    }
    return left;
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseAnd() {
    auto left = parseCmp();
    while (match(PineTok::KW_AND)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::BINARY_OP;
        node->op = "and";
        node->children = {left, parseCmp()};
        left = node;
    }
    return left;
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseCmp() {
    auto left = parseAdd();
    while (check(PineTok::EQ) || check(PineTok::NEQ) ||
           check(PineTok::LT) || check(PineTok::GT) ||
           check(PineTok::LTE) || check(PineTok::GTE)) {
        std::string op = cur().value;
        advance();
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::BINARY_OP;
        node->op = op;
        node->children = {left, parseAdd()};
        left = node;
    }
    return left;
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseAdd() {
    auto left = parseMul();
    while (check(PineTok::PLUS) || check(PineTok::MINUS)) {
        std::string op = cur().value;
        advance();
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::BINARY_OP;
        node->op = op;
        node->children = {left, parseMul()};
        left = node;
    }
    return left;
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseMul() {
    auto left = parseUnary();
    while (check(PineTok::STAR) || check(PineTok::SLASH) || check(PineTok::PERCENT)) {
        std::string op = cur().value;
        advance();
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::BINARY_OP;
        node->op = op;
        node->children = {left, parseUnary()};
        left = node;
    }
    return left;
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseUnary() {
    if (match(PineTok::MINUS)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::UNARY_OP;
        node->op = "-";
        node->children.push_back(parseUnary());
        return node;
    }
    if (match(PineTok::KW_NOT)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::UNARY_OP;
        node->op = "not";
        node->children.push_back(parseUnary());
        return node;
    }
    return parsePrimary();
}

std::shared_ptr<PineExpr> PineConverter::Parser::parsePrimary() {
    // Number literal
    if (check(PineTok::NUMBER)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::LITERAL_NUM;
        node->numVal = std::stod(cur().value);
        advance();
        return node;
    }

    // String literal
    if (check(PineTok::STRING)) {
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::LITERAL_STR;
        node->strVal = cur().value;
        advance();
        return node;
    }

    // Boolean / na
    if (match(PineTok::KW_TRUE)) {
        auto n = std::make_shared<PineExpr>();
        n->kind = PineExpr::Kind::LITERAL_BOOL; n->boolVal = true; return n;
    }
    if (match(PineTok::KW_FALSE)) {
        auto n = std::make_shared<PineExpr>();
        n->kind = PineExpr::Kind::LITERAL_BOOL; n->boolVal = false; return n;
    }
    if (match(PineTok::KW_NA)) {
        auto n = std::make_shared<PineExpr>();
        n->kind = PineExpr::Kind::LITERAL_NA; return n;
    }

    // Parenthesized expression
    if (match(PineTok::LPAREN)) {
        auto e = parseExpr();
        expect(PineTok::RPAREN);
        return e;
    }

    // Identifier (variable, function call, or dotted name like ta.ema)
    if (check(PineTok::IDENT)) {
        std::string name = cur().value;
        advance();

        // Dotted name: ta.ema, math.abs, input.int, color.red, etc.
        if (match(PineTok::DOT)) {
            if (check(PineTok::IDENT)) {
                std::string member = cur().value;
                advance();
                std::string full = name + "." + member;
                // Function call
                if (check(PineTok::LPAREN)) return parseCall(full);
                // Plain dotted value (e.g., color.red) — return as variable
                auto node = std::make_shared<PineExpr>();
                node->kind = PineExpr::Kind::VARIABLE;
                node->strVal = full;
                return node;
            }
        }

        // Function call
        if (check(PineTok::LPAREN)) return parseCall(name);

        // Series access: name[offset]
        if (match(PineTok::LBRACKET)) {
            auto node = std::make_shared<PineExpr>();
            node->kind = PineExpr::Kind::SERIES_ACCESS;
            node->strVal = name;
            node->children.push_back(parseExpr());
            expect(PineTok::RBRACKET);
            return node;
        }

        // Plain variable
        auto node = std::make_shared<PineExpr>();
        node->kind = PineExpr::Kind::VARIABLE;
        node->strVal = name;
        return node;
    }

    throw std::runtime_error("Pine parse error at line " +
        std::to_string(cur().line) + ": unexpected token '" + cur().value + "'");
}

std::shared_ptr<PineExpr> PineConverter::Parser::parseCall(const std::string& name) {
    auto node = std::make_shared<PineExpr>();
    node->kind = PineExpr::Kind::FUNC_CALL;
    node->strVal = name;
    expect(PineTok::LPAREN);
    while (!check(PineTok::RPAREN) && !check(PineTok::END_OF_FILE)) {
        // Check for named argument: ident = expr
        if (check(PineTok::IDENT)) {
            size_t save = pos_;
            std::string argName = cur().value;
            advance();
            if (match(PineTok::ASSIGN)) {
                node->namedArgs[argName] = parseExpr();
                if (!check(PineTok::RPAREN)) match(PineTok::COMMA);
                continue;
            }
            pos_ = save;
        }
        node->children.push_back(parseExpr());
        if (!check(PineTok::RPAREN)) match(PineTok::COMMA);
    }
    expect(PineTok::RPAREN);
    return node;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Public parse methods
// ═════════════════════════════════════════════════════════════════════════════
PineScript PineConverter::parseFile(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open())
        throw std::runtime_error("Cannot open .pine file: " + filepath);
    std::ostringstream ss;
    ss << f.rdbuf();
    return parseSource(ss.str());
}

PineScript PineConverter::parseSource(const std::string& source) {
    // Detect version from comments
    auto tokens = tokenize(source);
    Parser parser(std::move(tokens));
    auto script = parser.parse();

    // Check for //@version=N in source
    std::regex vr(R"(//@version=(\d+))");
    std::smatch m;
    if (std::regex_search(source, m, vr))
        script.version = std::stoi(m[1].str());

    return script;
}

// ═════════════════════════════════════════════════════════════════════════════
//  PineRuntime — interpreter
// ═════════════════════════════════════════════════════════════════════════════
void PineRuntime::load(const PineScript& s) { script_ = s; }

std::map<std::string, double> PineRuntime::update(const Candle& c) {
    cur_ = c;
    history_.push_back(c);
    if (history_.size() > 1000) history_.pop_front();
    plots_.clear();

    for (auto& stmt : script_.statements) exec(stmt);

    ++barIndex_;
    return plots_;
}

void PineRuntime::exec(const PineStmt& s) {
    switch (s.kind) {
        case PineStmt::Kind::ASSIGN:
        case PineStmt::Kind::VAR_DECL: {
            double v = eval(s.expr);
            vars_[s.name] = v;
            store(s.name, v);
            break;
        }
        case PineStmt::Kind::PLOT: {
            double v = eval(s.expr);
            plots_[s.plotTitle] = v;
            break;
        }
        default: break;
    }
}

double PineRuntime::eval(const std::shared_ptr<PineExpr>& e) {
    if (!e) return 0.0;
    switch (e->kind) {
        case PineExpr::Kind::LITERAL_NUM:  return e->numVal;
        case PineExpr::Kind::LITERAL_BOOL: return e->boolVal ? 1.0 : 0.0;
        case PineExpr::Kind::LITERAL_STR:  return 0.0;
        case PineExpr::Kind::LITERAL_NA:   return std::numeric_limits<double>::quiet_NaN();

        case PineExpr::Kind::VARIABLE: {
            const auto& n = e->strVal;
            if (n == "close")     return cur_.close;
            if (n == "open")      return cur_.open;
            if (n == "high")      return cur_.high;
            if (n == "low")       return cur_.low;
            if (n == "volume")    return cur_.volume;
            if (n == "hl2")       return (cur_.high + cur_.low) / 2.0;
            if (n == "hlc3")      return (cur_.high + cur_.low + cur_.close) / 3.0;
            if (n == "ohlc4")     return (cur_.open + cur_.high + cur_.low + cur_.close) / 4.0;
            if (n == "bar_index") return static_cast<double>(barIndex_);
            // syminfo.* defaults
            if (n == "syminfo.mintick")    return 0.01;
            if (n == "syminfo.pointvalue") return 1.0;
            // timeframe.* defaults
            if (n == "timeframe.multiplier") return 15.0;
            if (n == "timeframe.isintraday") return 1.0;
            if (n == "timeframe.isdaily")    return 0.0;
            if (n == "timeframe.isweekly")   return 0.0;
            if (n == "timeframe.ismonthly")  return 0.0;
            if (n == "timeframe.period")     return 15.0;
            // strategy.* defaults
            if (n == "strategy.equity")          return 1000.0;
            if (n == "strategy.initial_capital") return 1000.0;
            if (n == "strategy.closedtrades")    return 0.0;
            if (n == "strategy.opentrades")      return 0.0;
            if (n == "strategy.wintrades")       return 0.0;
            if (n == "strategy.grossprofit")     return 0.0;
            if (n == "strategy.grossloss")       return 0.0;
            if (n.find("strategy.") == 0)        return 0.0;
            // format.* constants
            if (n == "format.mintick") return 0.0;
            // Pine enums
            if (n == "order.ascending")  return 1.0;
            if (n == "order.descending") return -1.0;
            if (n == "xloc.bar_index")   return 0.0;
            if (n == "yloc.price")       return 0.0;
            if (n == "yloc.abovebar")    return 1.0;
            if (n == "yloc.belowbar")    return -1.0;
            if (n.find("color.") == 0)   return 0.0;
            if (n.find("line.style_") == 0)  return 0.0;
            if (n.find("label.style_") == 0) return 0.0;
            if (n.find("text.align_") == 0)  return 0.0;
            if (n.find("shape.") == 0)       return 0.0;
            if (n.find("location.") == 0)    return 0.0;
            if (n.find("display.") == 0)     return 0.0;
            if (n.find("size.") == 0)        return 0.0;
            auto it = vars_.find(n);
            return it != vars_.end() ? it->second : 0.0;
        }

        case PineExpr::Kind::SERIES_ACCESS: {
            int offset = static_cast<int>(eval(e->children[0]));
            return seriesAt(e->strVal, offset);
        }

        case PineExpr::Kind::UNARY_OP: {
            double v = eval(e->children[0]);
            if (e->op == "-")   return -v;
            if (e->op == "not") return v == 0.0 ? 1.0 : 0.0;
            return v;
        }

        case PineExpr::Kind::BINARY_OP: {
            double l = eval(e->children[0]);
            double r = eval(e->children[1]);
            if (e->op == "+")  return l + r;
            if (e->op == "-")  return l - r;
            if (e->op == "*")  return l * r;
            if (e->op == "/")  return r != 0.0 ? l / r : std::numeric_limits<double>::quiet_NaN();
            if (e->op == "%")  return r != 0.0 ? std::fmod(l, r) : std::numeric_limits<double>::quiet_NaN();
            if (e->op == ">")  return l > r  ? 1.0 : 0.0;
            if (e->op == "<")  return l < r  ? 1.0 : 0.0;
            if (e->op == ">=") return l >= r ? 1.0 : 0.0;
            if (e->op == "<=") return l <= r ? 1.0 : 0.0;
            if (e->op == "==") return l == r ? 1.0 : 0.0;
            if (e->op == "!=") return l != r ? 1.0 : 0.0;
            if (e->op == "and") return (l != 0.0 && r != 0.0) ? 1.0 : 0.0;
            if (e->op == "or")  return (l != 0.0 || r != 0.0) ? 1.0 : 0.0;
            return 0.0;
        }

        case PineExpr::Kind::TERNARY: {
            double cond = eval(e->children[0]);
            return cond != 0.0 ? eval(e->children[1]) : eval(e->children[2]);
        }

        case PineExpr::Kind::FUNC_CALL: {
            std::vector<double> args;
            for (auto& child : e->children) args.push_back(eval(child));
            return callFunc(e->strVal, args);
        }
    }
    return 0.0;
}

double PineRuntime::callFunc(const std::string& fn, const std::vector<double>& args) {
    // ta.ema(source, length)
    if (fn == "ta.ema" && args.size() >= 2) {
        int period = static_cast<int>(args[1]);
        std::string key = "ema_" + std::to_string(period);
        if (emas_.find(key) == emas_.end())
            emas_[key] = std::make_unique<EMA>(period);
        emas_[key]->updateValue(args[0]);
        return emas_[key]->value();
    }

    // ta.sma(source, length)
    if (fn == "ta.sma" && args.size() >= 2) {
        int period = static_cast<int>(args[1]);
        std::string key = "sma_" + std::to_string(period);
        auto& buf = series_["__sma_buf_" + key];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > period * 3)
            buf.pop_front();
        if (static_cast<int>(buf.size()) >= period) {
            double sum = 0.0;
            int n = static_cast<int>(buf.size());
            for (int i = n - period; i < n; ++i) sum += buf[static_cast<size_t>(i)];
            return sum / period;
        }
        return args[0];
    }

    // ta.rsi(source, length)
    if (fn == "ta.rsi" && args.size() >= 2) {
        int period = static_cast<int>(args[1]);
        std::string key = "rsi_" + std::to_string(period);
        if (rsis_.find(key) == rsis_.end())
            rsis_[key] = std::make_unique<RSI>(period);
        // RSI expects a Candle; create a dummy candle with close = source
        Candle dummy;
        dummy.close = args[0];
        rsis_[key]->update(dummy);
        return rsis_[key]->value();
    }

    // ta.atr(length)
    if (fn == "ta.atr" && args.size() >= 1) {
        int period = static_cast<int>(args[0]);
        std::string key = "atr_" + std::to_string(period);
        if (atrs_.find(key) == atrs_.end())
            atrs_[key] = std::make_unique<ATR>(period);
        atrs_[key]->update(cur_);
        return atrs_[key]->value();
    }

    // ta.crossover(a, b)
    if (fn == "ta.crossover" && args.size() >= 2) {
        double prevA = prevVals_.count("cross_a") ? prevVals_["cross_a"] : args[0];
        double prevB = prevVals_.count("cross_b") ? prevVals_["cross_b"] : args[1];
        bool result = ta::crossover(args[0], prevA, args[1], prevB);
        prevVals_["cross_a"] = args[0];
        prevVals_["cross_b"] = args[1];
        return result ? 1.0 : 0.0;
    }

    // ta.crossunder(a, b)
    if (fn == "ta.crossunder" && args.size() >= 2) {
        double prevA = prevVals_.count("xunder_a") ? prevVals_["xunder_a"] : args[0];
        double prevB = prevVals_.count("xunder_b") ? prevVals_["xunder_b"] : args[1];
        bool result = ta::crossunder(args[0], prevA, args[1], prevB);
        prevVals_["xunder_a"] = args[0];
        prevVals_["xunder_b"] = args[1];
        return result ? 1.0 : 0.0;
    }

    // ta.highest(source, length)
    if (fn == "ta.highest" && args.size() >= 2) {
        int len = static_cast<int>(args[1]);
        auto& buf = series_["__highest_buf"];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > len * 3)
            buf.pop_front();
        return ta::highest(buf, len);
    }

    // ta.lowest(source, length)
    if (fn == "ta.lowest" && args.size() >= 2) {
        int len = static_cast<int>(args[1]);
        auto& buf = series_["__lowest_buf"];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > len * 3)
            buf.pop_front();
        return ta::lowest(buf, len);
    }

    // math.abs(x)
    if (fn == "math.abs" && args.size() >= 1) return std::abs(args[0]);
    if (fn == "math.sqrt" && args.size() >= 1) return std::sqrt(args[0]);
    if (fn == "math.log" && args.size() >= 1) return std::log(args[0]);
    if (fn == "math.max" && args.size() >= 2) return std::max(args[0], args[1]);
    if (fn == "math.min" && args.size() >= 2) return std::min(args[0], args[1]);

    // nz(x, replacement)
    if (fn == "nz") {
        double val = args.size() >= 1 ? args[0] : 0.0;
        double rep = args.size() >= 2 ? args[1] : 0.0;
        return std::isnan(val) ? rep : val;
    }

    // input.int(defval, ...) / input.float(defval, ...) / input.source(defval, ...)
    // input.string(...) / input.bool(...) — return default or 0
    if (fn == "input.int" || fn == "input.float" || fn == "input.source" ||
        fn == "input.string" || fn == "input.bool") {
        return args.size() >= 1 ? args[0] : 0.0;
    }

    // math.round(x)
    if (fn == "math.round" && args.size() >= 1) return std::round(args[0]);
    // math.ceil(x)
    if (fn == "math.ceil" && args.size() >= 1) return std::ceil(args[0]);
    // math.floor(x)
    if (fn == "math.floor" && args.size() >= 1) return std::floor(args[0]);
    // math.pow(base, exp)
    if (fn == "math.pow" && args.size() >= 2) return std::pow(args[0], args[1]);
    // math.sign(x)
    if (fn == "math.sign" && args.size() >= 1)
        return args[0] > 0 ? 1.0 : args[0] < 0 ? -1.0 : 0.0;
    // math.avg(a, b)
    if (fn == "math.avg" && args.size() >= 2) return (args[0] + args[1]) / 2.0;

    // ta.stdev(source, length)
    if (fn == "ta.stdev" && args.size() >= 2) {
        int period = static_cast<int>(args[1]);
        std::string key = "__stdev_buf_" + std::to_string(period);
        auto& buf = series_[key];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > period * 3) buf.pop_front();
        int n = static_cast<int>(buf.size());
        if (n >= period) {
            double sum = 0.0;
            for (int i = n - period; i < n; ++i) sum += buf[static_cast<size_t>(i)];
            double mean = sum / period;
            double var = 0.0;
            for (int i = n - period; i < n; ++i) {
                double d = buf[static_cast<size_t>(i)] - mean;
                var += d * d;
            }
            return std::sqrt(var / period);
        }
        return 0.0;
    }

    // ta.change(source, length) — defaults to length=1
    if (fn == "ta.change") {
        if (args.size() >= 1) {
            int len = args.size() >= 2 ? static_cast<int>(args[1]) : 1;
            std::string key = "__change_buf";
            auto& buf = series_[key];
            buf.push_back(args[0]);
            if (static_cast<int>(buf.size()) > 1000) buf.pop_front();
            int idx = static_cast<int>(buf.size()) - 1 - len;
            if (idx >= 0) return args[0] - buf[static_cast<size_t>(idx)];
        }
        return 0.0;
    }

    // ta.vwap(source) — volume-weighted average price
    if (fn == "ta.vwap") {
        auto& cumPV = series_["__vwap_pv"];
        auto& cumV  = series_["__vwap_v"];
        double src = args.size() >= 1 ? args[0] : cur_.close;
        double pv = (cumPV.empty() ? 0.0 : cumPV.back()) + src * cur_.volume;
        double sv = (cumV.empty()  ? 0.0 : cumV.back())  + cur_.volume;
        cumPV.push_back(pv);
        cumV.push_back(sv);
        if (cumPV.size() > 1000) { cumPV.pop_front(); cumV.pop_front(); }
        return sv > 0.0 ? pv / sv : src;
    }

    // ta.highestbars(source, length) — bars since highest value
    if (fn == "ta.highestbars" && args.size() >= 2) {
        int len = static_cast<int>(args[1]);
        auto& buf = series_["__highestbars_buf"];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > len * 3) buf.pop_front();
        int n = static_cast<int>(buf.size());
        int start = std::max(0, n - len);
        double maxV = buf[static_cast<size_t>(start)];
        int maxIdx = start;
        for (int i = start + 1; i < n; ++i) {
            if (buf[static_cast<size_t>(i)] >= maxV) { maxV = buf[static_cast<size_t>(i)]; maxIdx = i; }
        }
        return static_cast<double>(maxIdx - (n - 1)); // negative offset
    }

    // ta.lowestbars(source, length) — bars since lowest value
    if (fn == "ta.lowestbars" && args.size() >= 2) {
        int len = static_cast<int>(args[1]);
        auto& buf = series_["__lowestbars_buf"];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > len * 3) buf.pop_front();
        int n = static_cast<int>(buf.size());
        int start = std::max(0, n - len);
        double minV = buf[static_cast<size_t>(start)];
        int minIdx = start;
        for (int i = start + 1; i < n; ++i) {
            if (buf[static_cast<size_t>(i)] <= minV) { minV = buf[static_cast<size_t>(i)]; minIdx = i; }
        }
        return static_cast<double>(minIdx - (n - 1)); // negative offset
    }

    // ta.pivothigh(source, leftbars, rightbars) — returns pivot high value or NaN
    if (fn == "ta.pivothigh" && args.size() >= 3) {
        int leftBars  = static_cast<int>(args[1]);
        int rightBars = static_cast<int>(args[2]);
        auto& buf = series_["__pivothigh_buf"];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > 1000) buf.pop_front();
        int n = static_cast<int>(buf.size());
        int pivotIdx = n - 1 - rightBars;
        if (pivotIdx < leftBars || pivotIdx < 0) return std::numeric_limits<double>::quiet_NaN();
        double pivotVal = buf[static_cast<size_t>(pivotIdx)];
        for (int i = pivotIdx - leftBars; i < pivotIdx; ++i)
            if (buf[static_cast<size_t>(i)] >= pivotVal) return std::numeric_limits<double>::quiet_NaN();
        for (int i = pivotIdx + 1; i <= pivotIdx + rightBars && i < n; ++i)
            if (buf[static_cast<size_t>(i)] >= pivotVal) return std::numeric_limits<double>::quiet_NaN();
        return pivotVal;
    }

    // ta.pivotlow(source, leftbars, rightbars) — returns pivot low value or NaN
    if (fn == "ta.pivotlow" && args.size() >= 3) {
        int leftBars  = static_cast<int>(args[1]);
        int rightBars = static_cast<int>(args[2]);
        auto& buf = series_["__pivotlow_buf"];
        buf.push_back(args[0]);
        if (static_cast<int>(buf.size()) > 1000) buf.pop_front();
        int n = static_cast<int>(buf.size());
        int pivotIdx = n - 1 - rightBars;
        if (pivotIdx < leftBars || pivotIdx < 0) return std::numeric_limits<double>::quiet_NaN();
        double pivotVal = buf[static_cast<size_t>(pivotIdx)];
        for (int i = pivotIdx - leftBars; i < pivotIdx; ++i)
            if (buf[static_cast<size_t>(i)] <= pivotVal) return std::numeric_limits<double>::quiet_NaN();
        for (int i = pivotIdx + 1; i <= pivotIdx + rightBars && i < n; ++i)
            if (buf[static_cast<size_t>(i)] <= pivotVal) return std::numeric_limits<double>::quiet_NaN();
        return pivotVal;
    }

    // ta.dmi(diLength, adxSmoothing) — returns ADX value
    if (fn == "ta.dmi" && args.size() >= 2) {
        int len = static_cast<int>(args[0]);
        auto& hiB = series_["__dmi_hi"];
        auto& loB = series_["__dmi_lo"];
        auto& adxB = series_["__dmi_adx"];
        hiB.push_back(cur_.high);
        loB.push_back(cur_.low);
        if (hiB.size() > 1000) { hiB.pop_front(); loB.pop_front(); }
        int n = static_cast<int>(hiB.size());
        if (n < 2) { adxB.push_back(0); return 0.0; }
        double upMove = hiB[static_cast<size_t>(n-1)] - hiB[static_cast<size_t>(n-2)];
        double downMove = loB[static_cast<size_t>(n-2)] - loB[static_cast<size_t>(n-1)];
        double plusDM  = (upMove > downMove && upMove > 0) ? upMove : 0;
        double minusDM = (downMove > upMove && downMove > 0) ? downMove : 0;
        double prevClose = (history_.size() >= 2)
                           ? history_[history_.size()-2].close : cur_.close;
        double tr = std::max(cur_.high - cur_.low,
                    std::max(std::abs(cur_.high - prevClose),
                             std::abs(cur_.low  - prevClose)));
        if (tr < 1e-12) tr = 1e-12;
        double diPlus  = (plusDM / tr) * 100.0;
        double diMinus = (minusDM / tr) * 100.0;
        double dx = (diPlus + diMinus > 0)
                    ? std::abs(diPlus - diMinus) / (diPlus + diMinus) * 100.0
                    : 0.0;
        double prevAdx = adxB.empty() ? dx : adxB.back();
        double adx = (prevAdx * (len - 1) + dx) / len;
        adxB.push_back(adx);
        if (adxB.size() > 1000) adxB.pop_front();
        return adx;
    }

    // math.exp(x)
    if (fn == "math.exp" && args.size() >= 1) return std::exp(args[0]);

    // str.tostring(v, ...) / str.length(...) — return 0 in numeric context
    if (fn.find("str.") == 0) return 0.0;

    // strategy.* calls — ignore in indicator context
    if (fn.find("strategy.") == 0) return 0.0;

    // color.* — return 0 (not used numerically)
    if (fn.find("color.") == 0) return 0.0;

    // Visual/drawing functions — no-ops in C++ runtime
    if (fn.find("label.") == 0 || fn.find("line.") == 0 ||
        fn.find("box.") == 0 || fn.find("linefill.") == 0 ||
        fn.find("table.") == 0 || fn.find("array.") == 0)
        return args.empty() ? 0.0 : args[0];

    // plotshape, plotchar, plotarrow, bgcolor, barcolor, fill, hline — visual no-ops
    if (fn == "plotshape" || fn == "plotchar" || fn == "plotarrow" ||
        fn == "bgcolor" || fn == "barcolor" || fn == "fill" || fn == "hline" ||
        fn == "alertcondition")
        return 0.0;

    // input.color, input.session, input.time, input.timeframe — return default
    if (fn.find("input.") == 0)
        return args.size() >= 1 ? args[0] : 0.0;

    // timeframe.* — return approximate values
    if (fn == "timeframe.in_seconds") return 900.0; // 15m default

    return 0.0;
}

double PineRuntime::seriesAt(const std::string& name, int offset) const {
    // Check built-in series
    if (name == "close" || name == "open" || name == "high" ||
        name == "low" || name == "volume") {
        int idx = static_cast<int>(history_.size()) - 1 - offset;
        if (idx < 0 || idx >= static_cast<int>(history_.size())) return 0.0;
        const auto& candle = history_[static_cast<size_t>(idx)];
        if (name == "close")  return candle.close;
        if (name == "open")   return candle.open;
        if (name == "high")   return candle.high;
        if (name == "low")    return candle.low;
        if (name == "volume") return candle.volume;
    }
    // User series
    auto it = series_.find(name);
    if (it == series_.end()) return 0.0;
    int idx = static_cast<int>(it->second.size()) - 1 - offset;
    if (idx < 0 || idx >= static_cast<int>(it->second.size())) return 0.0;
    return it->second[static_cast<size_t>(idx)];
}

void PineRuntime::store(const std::string& name, double v) {
    series_[name].push_back(v);
    if (series_[name].size() > 1000) series_[name].pop_front();
}

// ═════════════════════════════════════════════════════════════════════════════
//  C++ Code Generator
// ═════════════════════════════════════════════════════════════════════════════
std::string PineConverter::generateCpp(const PineScript& script,
                                       const std::string& className) {
    std::ostringstream out;
    out << "// Auto-generated C++ indicator from Pine Script v" << script.version << ": "
        << script.name << "\n"
        << "// This indicator reads candle (bar) data and computes values.\n"
        << "#pragma once\n"
        << "#include \"EMA.h\"\n"
        << "#include \"RSI.h\"\n"
        << "#include \"ATR.h\"\n"
        << "#include \"PineScriptIndicator.h\"\n"
        << "#include <map>\n#include <string>\n#include <cmath>\n"
        << "#include <deque>\n#include <numeric>\n#include <algorithm>\n\n"
        << "namespace crypto {\n\n"
        << "class " << className << " {\n"
        << "public:\n";

    // Collect indicator instances needed & build plot-to-variable mapping
    struct IndInfo { std::string type; std::string key; int period; };
    std::vector<IndInfo> inds;
    struct PlotInfo { std::string title; std::string sourceVar; };
    std::vector<PlotInfo> plotInfos;

    // Also collect simple variable assignments (numeric literals, expressions)
    struct VarInfo { std::string name; double defaultVal; };
    std::vector<VarInfo> simpleVars;

    // Collect SMA / crossover / crossunder / highest / lowest / stdev / change / vwap / macd / bb / dmi calls
    struct SmaInfo { std::string key; int period; };
    std::vector<SmaInfo> smaInfos;
    struct CrossInfo { std::string key; std::string srcA; std::string srcB; bool isOver; };
    std::vector<CrossInfo> crossInfos;
    struct SeriesFuncInfo { std::string key; std::string func; int period; };
    std::vector<SeriesFuncInfo> seriesFuncs; // highest, lowest, stdev, change, highestbars, lowestbars, pivothigh, pivotlow
    struct MacdInfo { std::string key; int fast; int slow; int signal; };
    std::vector<MacdInfo> macdInfos;
    struct BbInfo { std::string key; int period; double stddev; };
    std::vector<BbInfo> bbInfos;
    struct DmiInfo { std::string key; int diLength; int adxSmoothing; };
    std::vector<DmiInfo> dmiInfos;
    bool needsVwap = false;
    std::string vwapKey;

    // Strategy entry/exit/close calls
    struct StrategyCall { std::string action; std::string id; std::string direction; };
    std::vector<StrategyCall> strategyCalls;

    for (auto& stmt : script.statements) {
        if (stmt.kind == PineStmt::Kind::ASSIGN ||
            stmt.kind == PineStmt::Kind::VAR_DECL) {
            if (stmt.expr && stmt.expr->kind == PineExpr::Kind::FUNC_CALL) {
                const auto& fn = stmt.expr->strVal;
                int period = 14;
                if (stmt.expr->children.size() >= 2)
                    period = static_cast<int>(stmt.expr->children[1]->numVal);
                else if (stmt.expr->children.size() >= 1)
                    period = static_cast<int>(stmt.expr->children[0]->numVal);

                if (fn == "ta.ema") inds.push_back({"EMA", stmt.name, period});
                else if (fn == "ta.rsi") inds.push_back({"RSI", stmt.name, period});
                else if (fn == "ta.atr") inds.push_back({"ATR", stmt.name, period});
                else if (fn == "ta.sma") smaInfos.push_back({stmt.name, period});
                else if (fn == "ta.crossover" && stmt.expr->children.size() >= 2) {
                    std::string a = stmt.expr->children[0]->strVal;
                    std::string b = stmt.expr->children[1]->strVal;
                    crossInfos.push_back({stmt.name, a, b, true});
                }
                else if (fn == "ta.crossunder" && stmt.expr->children.size() >= 2) {
                    std::string a = stmt.expr->children[0]->strVal;
                    std::string b = stmt.expr->children[1]->strVal;
                    crossInfos.push_back({stmt.name, a, b, false});
                }
                else if (fn == "ta.highest" || fn == "ta.lowest" ||
                         fn == "ta.stdev" || fn == "ta.change" ||
                         fn == "ta.highestbars" || fn == "ta.lowestbars" ||
                         fn == "ta.pivothigh" || fn == "ta.pivotlow") {
                    seriesFuncs.push_back({stmt.name, fn, period});
                }
                else if (fn == "ta.macd" && stmt.expr->children.size() >= 3) {
                    int f = static_cast<int>(stmt.expr->children[0]->numVal);
                    int s = static_cast<int>(stmt.expr->children[1]->numVal);
                    int sig = static_cast<int>(stmt.expr->children[2]->numVal);
                    if (f <= 0) f = 12;
                    if (s <= 0) s = 26;
                    if (sig <= 0) sig = 9;
                    macdInfos.push_back({stmt.name, f, s, sig});
                }
                else if (fn == "ta.bb" && stmt.expr->children.size() >= 2) {
                    int p = static_cast<int>(stmt.expr->children[0]->numVal);
                    double sd = stmt.expr->children.size() >= 3
                                ? stmt.expr->children[2]->numVal : 2.0;
                    if (p <= 0) p = 20;
                    if (sd <= 0.0) sd = 2.0;
                    bbInfos.push_back({stmt.name, p, sd});
                }
                else if (fn == "ta.vwap") {
                    needsVwap = true;
                    vwapKey = stmt.name;
                }
                else if (fn == "ta.dmi" && stmt.expr->children.size() >= 2) {
                    int diLen = static_cast<int>(stmt.expr->children[0]->numVal);
                    int adxSmooth = static_cast<int>(stmt.expr->children[1]->numVal);
                    if (diLen <= 0) diLen = 14;
                    if (adxSmooth <= 0) adxSmooth = 14;
                    dmiInfos.push_back({stmt.name, diLen, adxSmooth});
                }
            } else if (stmt.expr && stmt.expr->kind == PineExpr::Kind::LITERAL_NUM) {
                simpleVars.push_back({stmt.name, stmt.expr->numVal});
            }
        }
        if (stmt.kind == PineStmt::Kind::PLOT) {
            std::string src;
            // Determine which variable feeds this plot
            if (stmt.expr && stmt.expr->kind == PineExpr::Kind::VARIABLE)
                src = stmt.expr->strVal;
            else if (stmt.expr && stmt.expr->kind == PineExpr::Kind::FUNC_CALL &&
                     !stmt.expr->children.empty() &&
                     stmt.expr->children[0]->kind == PineExpr::Kind::VARIABLE)
                src = stmt.expr->children[0]->strVal;
            plotInfos.push_back({stmt.plotTitle, src});
        }
        // Detect strategy.entry / strategy.exit / strategy.close calls
        if (stmt.kind == PineStmt::Kind::EXPR && stmt.expr &&
            stmt.expr->kind == PineExpr::Kind::FUNC_CALL) {
            const auto& fn = stmt.expr->strVal;
            if (fn == "strategy.entry" || fn == "strategy.exit" || fn == "strategy.close") {
                std::string action;
                if (fn == "strategy.entry") action = "entry";
                else if (fn == "strategy.exit") action = "exit";
                else action = "close";
                std::string id;
                if (!stmt.expr->children.empty() && stmt.expr->children[0]->kind == PineExpr::Kind::LITERAL_STR)
                    id = stmt.expr->children[0]->strVal;
                else if (!stmt.expr->children.empty())
                    id = stmt.expr->children[0]->strVal;
                // Parse direction from second argument (strategy.long / strategy.short)
                std::string direction;
                if (stmt.expr->children.size() >= 2) {
                    const auto& dirArg = stmt.expr->children[1];
                    if (dirArg->kind == PineExpr::Kind::VARIABLE) {
                        if (dirArg->strVal == "strategy.long" || dirArg->strVal == "long")
                            direction = "long";
                        else if (dirArg->strVal == "strategy.short" || dirArg->strVal == "short")
                            direction = "short";
                    }
                }
                // Also check named args
                if (direction.empty() && stmt.expr->namedArgs.count("direction")) {
                    const auto& dirArg = stmt.expr->namedArgs.at("direction");
                    if (dirArg->kind == PineExpr::Kind::VARIABLE) {
                        if (dirArg->strVal == "strategy.long" || dirArg->strVal == "long")
                            direction = "long";
                        else if (dirArg->strVal == "strategy.short" || dirArg->strVal == "short")
                            direction = "short";
                    }
                }
                strategyCalls.push_back({action, id, direction});
            }
        }
        // Also detect strategy calls as assign statements (strategy.entry used inline)
        if ((stmt.kind == PineStmt::Kind::ASSIGN || stmt.kind == PineStmt::Kind::VAR_DECL) &&
            stmt.expr && stmt.expr->kind == PineExpr::Kind::FUNC_CALL) {
            const auto& fn = stmt.expr->strVal;
            if (fn == "strategy.entry" || fn == "strategy.exit" || fn == "strategy.close") {
                std::string action;
                if (fn == "strategy.entry") action = "entry";
                else if (fn == "strategy.exit") action = "exit";
                else action = "close";
                std::string id;
                if (!stmt.expr->children.empty() && stmt.expr->children[0]->kind == PineExpr::Kind::LITERAL_STR)
                    id = stmt.expr->children[0]->strVal;
                std::string direction;
                if (stmt.expr->children.size() >= 2) {
                    const auto& dirArg = stmt.expr->children[1];
                    if (dirArg->kind == PineExpr::Kind::VARIABLE) {
                        if (dirArg->strVal == "strategy.long" || dirArg->strVal == "long")
                            direction = "long";
                        else if (dirArg->strVal == "strategy.short" || dirArg->strVal == "short")
                            direction = "short";
                    }
                }
                strategyCalls.push_back({action, id, direction});
            }
        }
    }

    // Collect all known variable names for plot resolution
    auto isKnownVar = [&](const std::string& name) -> bool {
        for (auto& ind : inds) if (ind.key == name) return true;
        for (auto& sv : simpleVars) if (sv.name == name) return true;
        for (auto& s : smaInfos) if (s.key == name) return true;
        for (auto& c : crossInfos) if (c.key == name) return true;
        for (auto& sf : seriesFuncs) if (sf.key == name) return true;
        for (auto& m : macdInfos) if (m.key == name) return true;
        for (auto& b : bbInfos) if (b.key == name) return true;
        for (auto& d : dmiInfos) if (d.key == name) return true;
        if (needsVwap && vwapKey == name) return true;
        return false;
    };

    // Constructor
    out << "    " << className << "()\n        : ";
    bool first = true;
    for (auto& ind : inds) {
        if (!first) out << ", ";
        out << ind.key << "_(" << ind.period << ")";
        first = false;
    }
    for (auto& m : macdInfos) {
        if (!first) out << ", ";
        out << m.key << "_fast_(" << m.fast << "), "
            << m.key << "_slow_(" << m.slow << "), "
            << m.key << "_sig_(" << m.signal << ")";
        first = false;
    }
    if (first) out << "barIndex_(0)";
    else out << ", barIndex_(0)";
    out << " {}\n\n";

    // Update method — reads candle data and computes indicator values
    out << "    /// Update indicator with new candle bar data\n"
        << "    void update(const Candle& c) {\n"
        << "        ++barIndex_;\n"
        << "        closes_.push_back(c.close);\n"
        << "        if (closes_.size() > 1000) closes_.pop_front();\n";

    for (auto& ind : inds) {
        if (ind.type == "ATR")
            out << "        " << ind.key << "_.update(c);\n";
        else if (ind.type == "RSI") {
            out << "        { Candle rc{}; rc.close = c.close; rc.open = c.open; rc.high = c.high; rc.low = c.low; "
                << ind.key << "_.update(rc); }\n";
        } else
            out << "        " << ind.key << "_.updateValue(c.close);\n";
    }
    // MACD: update fast/slow EMAs, compute macd/signal/histogram
    for (auto& m : macdInfos) {
        out << "        " << m.key << "_fast_.updateValue(c.close);\n"
            << "        " << m.key << "_slow_.updateValue(c.close);\n"
            << "        if (" << m.key << "_fast_.ready() && " << m.key << "_slow_.ready()) {\n"
            << "            double macdLine = " << m.key << "_fast_.value() - " << m.key << "_slow_.value();\n"
            << "            " << m.key << "_sig_.updateValue(macdLine);\n"
            << "            " << m.key << "_macd_ = macdLine;\n"
            << "            " << m.key << "_signal_ = " << m.key << "_sig_.ready() ? " << m.key << "_sig_.value() : 0.0;\n"
            << "            " << m.key << "_hist_ = macdLine - " << m.key << "_signal_;\n"
            << "        }\n";
    }
    out << "\n";

    // Read indicator values
    for (auto& ind : inds)
        out << "        double " << ind.key << " = " << ind.key << "_.value();\n";
    for (auto& sv : simpleVars)
        out << "        double " << sv.name << " = " << sv.defaultVal << ";\n";

    // SMA computation
    for (auto& s : smaInfos) {
        out << "        double " << s.key << " = 0.0;\n"
            << "        if (static_cast<int>(closes_.size()) >= " << s.period << ") {\n"
            << "            double sum = 0.0;\n"
            << "            int n = static_cast<int>(closes_.size());\n"
            << "            for (int i = n - " << s.period << "; i < n; ++i) sum += closes_[static_cast<size_t>(i)];\n"
            << "            " << s.key << " = sum / " << s.period << ".0;\n"
            << "        } else { " << s.key << " = c.close; }\n";
    }

    // Crossover/Crossunder
    for (auto& cr : crossInfos) {
        std::string prevA = "prev_" + cr.srcA + "_";
        std::string prevB = "prev_" + cr.srcB + "_";
        if (cr.isOver) {
            out << "        double " << cr.key << " = (" << prevA << " <= " << prevB
                << " && " << cr.srcA << " > " << cr.srcB << ") ? 1.0 : 0.0;\n";
        } else {
            out << "        double " << cr.key << " = (" << prevA << " >= " << prevB
                << " && " << cr.srcA << " < " << cr.srcB << ") ? 1.0 : 0.0;\n";
        }
    }

    // Series functions: highest, lowest, stdev, change
    for (auto& sf : seriesFuncs) {
        if (sf.func == "ta.highest") {
            out << "        double " << sf.key << " = c.close;\n"
                << "        { int n = static_cast<int>(closes_.size()); int start = std::max(0, n - " << sf.period << ");\n"
                << "          for (int i = start; i < n; ++i) " << sf.key << " = std::max(" << sf.key << ", closes_[static_cast<size_t>(i)]); }\n";
        } else if (sf.func == "ta.lowest") {
            out << "        double " << sf.key << " = c.close;\n"
                << "        { int n = static_cast<int>(closes_.size()); int start = std::max(0, n - " << sf.period << ");\n"
                << "          for (int i = start; i < n; ++i) " << sf.key << " = std::min(" << sf.key << ", closes_[static_cast<size_t>(i)]); }\n";
        } else if (sf.func == "ta.stdev") {
            out << "        double " << sf.key << " = 0.0;\n"
                << "        { int n = static_cast<int>(closes_.size());\n"
                << "          if (n >= " << sf.period << ") {\n"
                << "            double sum = 0.0; for (int i = n - " << sf.period << "; i < n; ++i) sum += closes_[static_cast<size_t>(i)];\n"
                << "            double mean = sum / " << sf.period << ".0; double var = 0.0;\n"
                << "            for (int i = n - " << sf.period << "; i < n; ++i) { double d = closes_[static_cast<size_t>(i)] - mean; var += d*d; }\n"
                << "            " << sf.key << " = std::sqrt(var / " << sf.period << ".0);\n"
                << "          } }\n";
        } else if (sf.func == "ta.change") {
            int len = (sf.period > 0) ? sf.period : 1;
            out << "        double " << sf.key << " = 0.0;\n"
                << "        { int n = static_cast<int>(closes_.size()); int idx = n - 1 - " << len << ";\n"
                << "          if (idx >= 0) " << sf.key << " = c.close - closes_[static_cast<size_t>(idx)]; }\n";
        }
    }

    // Bollinger Bands
    for (auto& b : bbInfos) {
        out << "        double " << b.key << "_upper = 0.0, " << b.key << "_middle = 0.0, " << b.key << "_lower = 0.0;\n"
            << "        { int n = static_cast<int>(closes_.size());\n"
            << "          if (n >= " << b.period << ") {\n"
            << "            double sum = 0.0; for (int i = n - " << b.period << "; i < n; ++i) sum += closes_[static_cast<size_t>(i)];\n"
            << "            double mean = sum / " << b.period << ".0; double var = 0.0;\n"
            << "            for (int i = n - " << b.period << "; i < n; ++i) { double d = closes_[static_cast<size_t>(i)] - mean; var += d*d; }\n"
            << "            double sd = std::sqrt(var / " << b.period << ".0);\n"
            << "            " << b.key << "_middle = mean; " << b.key << "_upper = mean + " << b.stddev << " * sd;\n"
            << "            " << b.key << "_lower = mean - " << b.stddev << " * sd;\n"
            << "          } }\n";
    }

    // MACD value assignment
    for (auto& m : macdInfos) {
        out << "        double " << m.key << "_macd = " << m.key << "_macd_;\n"
            << "        double " << m.key << "_signal = " << m.key << "_signal_;\n"
            << "        double " << m.key << "_hist = " << m.key << "_hist_;\n";
    }

    // VWAP
    if (needsVwap) {
        out << "        cumPV_ += c.close * c.volume; cumVol_ += c.volume;\n"
            << "        double " << vwapKey << " = (cumVol_ > 0.0) ? cumPV_ / cumVol_ : c.close;\n";
    }

    // DMI (Directional Movement Index)
    // NOTE: ADX uses simplified single-period DX calculation.
    // Full ADX requires EMA smoothing over adxSmoothing periods;
    // this approximation provides directional strength without
    // the smoothing lag — adequate for indicator generation.
    for (auto& d : dmiInfos) {
        out << "        // DMI: +DI, -DI, ADX  (diLength=" << d.diLength << ", adxSmoothing=" << d.adxSmoothing << ")\n"
            << "        // NOTE: ADX is single-period DX (simplified, no EMA smoothing)\n"
            << "        highs_.push_back(c.high);\n"
            << "        lows_.push_back(c.low);\n"
            << "        if (highs_.size() > 1000) highs_.pop_front();\n"
            << "        if (lows_.size() > 1000) lows_.pop_front();\n"
            << "        double " << d.key << "_diPlus = 0.0, " << d.key << "_diMinus = 0.0, " << d.key << "_adx = 0.0;\n"
            << "        { int n = static_cast<int>(highs_.size());\n"
            << "          if (n > " << d.diLength << ") {\n"
            << "            double smPlusDM = 0.0, smMinusDM = 0.0, smTR = 0.0;\n"
            << "            for (int i = 1; i <= " << d.diLength << "; ++i) {\n"
            << "              double hi = highs_[static_cast<size_t>(n - " << d.diLength << " - 1 + i)];\n"
            << "              double lo = lows_[static_cast<size_t>(n - " << d.diLength << " - 1 + i)];\n"
            << "              double phi = highs_[static_cast<size_t>(n - " << d.diLength << " - 1 + i - 1)];\n"
            << "              double plo = lows_[static_cast<size_t>(n - " << d.diLength << " - 1 + i - 1)];\n"
            << "              double pclose = closes_[static_cast<size_t>(n - " << d.diLength << " - 1 + i - 1)];\n"
            << "              double upMove = hi - phi; double downMove = plo - lo;\n"
            << "              double plusDM = (upMove > downMove && upMove > 0) ? upMove : 0.0;\n"
            << "              double minusDM = (downMove > upMove && downMove > 0) ? downMove : 0.0;\n"
            << "              double tr = std::max({hi - lo, std::abs(hi - pclose), std::abs(lo - pclose)});\n"
            << "              smPlusDM += plusDM; smMinusDM += minusDM; smTR += tr;\n"
            << "            }\n"
            << "            if (smTR > 0.0) {\n"
            << "              " << d.key << "_diPlus = 100.0 * smPlusDM / smTR;\n"
            << "              " << d.key << "_diMinus = 100.0 * smMinusDM / smTR;\n"
            << "              double dx = std::abs(" << d.key << "_diPlus - " << d.key << "_diMinus) / (" << d.key << "_diPlus + " << d.key << "_diMinus + 1e-10) * 100.0;\n"
            << "              " << d.key << "_adx = dx; // Simplified: use single-period DX as ADX approximation\n"
            << "            }\n"
            << "          } }\n"
            << "        double " << d.key << " = " << d.key << "_adx;\n";
    }

    // Series functions: highestbars, lowestbars, pivothigh, pivotlow
    for (auto& sf : seriesFuncs) {
        if (sf.func == "ta.highestbars") {
            out << "        double " << sf.key << " = 0.0;\n"
                << "        { int n = static_cast<int>(closes_.size()); int start = std::max(0, n - " << sf.period << ");\n"
                << "          double mx = -1e18; int mxi = n - 1;\n"
                << "          for (int i = start; i < n; ++i) { if (closes_[static_cast<size_t>(i)] >= mx) { mx = closes_[static_cast<size_t>(i)]; mxi = i; } }\n"
                << "          " << sf.key << " = static_cast<double>(mxi - (n - 1)); }\n";
        } else if (sf.func == "ta.lowestbars") {
            out << "        double " << sf.key << " = 0.0;\n"
                << "        { int n = static_cast<int>(closes_.size()); int start = std::max(0, n - " << sf.period << ");\n"
                << "          double mn = 1e18; int mni = n - 1;\n"
                << "          for (int i = start; i < n; ++i) { if (closes_[static_cast<size_t>(i)] <= mn) { mn = closes_[static_cast<size_t>(i)]; mni = i; } }\n"
                << "          " << sf.key << " = static_cast<double>(mni - (n - 1)); }\n";
        } else if (sf.func == "ta.pivothigh") {
            int bars = sf.period;
            out << "        double " << sf.key << " = 0.0/0.0; // NaN = no pivot\n"
                << "        { int n = static_cast<int>(closes_.size()); int lb = " << bars << "; int rb = " << bars << ";\n"
                << "          if (n >= lb + rb + 1) {\n"
                << "            int pivotIdx = n - 1 - rb; double pivotVal = closes_[static_cast<size_t>(pivotIdx)]; bool isPivot = true;\n"
                << "            for (int i = pivotIdx - lb; i < pivotIdx; ++i) { if (closes_[static_cast<size_t>(i)] >= pivotVal) { isPivot = false; break; } }\n"
                << "            if (isPivot) for (int i = pivotIdx + 1; i <= pivotIdx + rb; ++i) { if (closes_[static_cast<size_t>(i)] >= pivotVal) { isPivot = false; break; } }\n"
                << "            if (isPivot) " << sf.key << " = pivotVal;\n"
                << "          } }\n";
        } else if (sf.func == "ta.pivotlow") {
            int bars = sf.period;
            out << "        double " << sf.key << " = 0.0/0.0; // NaN = no pivot\n"
                << "        { int n = static_cast<int>(closes_.size()); int lb = " << bars << "; int rb = " << bars << ";\n"
                << "          if (n >= lb + rb + 1) {\n"
                << "            int pivotIdx = n - 1 - rb; double pivotVal = closes_[static_cast<size_t>(pivotIdx)]; bool isPivot = true;\n"
                << "            for (int i = pivotIdx - lb; i < pivotIdx; ++i) { if (closes_[static_cast<size_t>(i)] <= pivotVal) { isPivot = false; break; } }\n"
                << "            if (isPivot) for (int i = pivotIdx + 1; i <= pivotIdx + rb; ++i) { if (closes_[static_cast<size_t>(i)] <= pivotVal) { isPivot = false; break; } }\n"
                << "            if (isPivot) " << sf.key << " = pivotVal;\n"
                << "          } }\n";
        }
    }

    out << "        (void)barIndex_; // bar count available for threshold checks\n";

    // Generate strategy entry/exit/close signal methods
    if (!strategyCalls.empty()) {
        out << "\n        // Strategy signals\n"
            << "        signal_ = 0; // 0=hold, 1=entry_long, -1=entry_short, 2=exit_long, -2=exit_short, 3=close_all\n";
        for (size_t si = 0; si < strategyCalls.size(); ++si) {
            const auto& sc = strategyCalls[si];
            if (sc.action == "entry") {
                // Use parsed direction if available, otherwise fallback to heuristic
                bool isLong = false;
                if (!sc.direction.empty()) {
                    isLong = (sc.direction == "long");
                } else {
                    // Fallback heuristic: determine direction from id string keywords
                    isLong = (sc.id.find("Long") != std::string::npos ||
                              sc.id.find("long") != std::string::npos ||
                              sc.id.find("Buy") != std::string::npos ||
                              sc.id.find("buy") != std::string::npos);
                }
                out << "        // strategy.entry(\"" << sc.id << "\", " << (isLong ? "strategy.long" : "strategy.short") << ")\n"
                    << "        // Set signal_=" << (isLong ? "1" : "-1") << " when entry condition is met\n";
            } else if (sc.action == "exit") {
                out << "        // strategy.exit(\"" << sc.id << "\")\n"
                    << "        // Set signal_=2 or -2 when exit condition is met\n";
            } else if (sc.action == "close") {
                out << "        // strategy.close(\"" << sc.id << "\")\n"
                    << "        // Set signal_=3 when close condition is met\n";
            }
        }
    }

    // Save previous values for crossover/crossunder detection
    for (auto& cr : crossInfos) {
        out << "        prev_" << cr.srcA << "_ = " << cr.srcA << ";\n"
            << "        prev_" << cr.srcB << "_ = " << cr.srcB << ";\n";
    }

    // Wire plots to their source variables
    out << "\n        plots_.clear();\n";
    for (auto& p : plotInfos) {
        if (!p.sourceVar.empty()) {
            if (isKnownVar(p.sourceVar))
                out << "        plots_[\"" << p.title << "\"] = " << p.sourceVar << ";\n";
            else
                out << "        plots_[\"" << p.title << "\"] = 0.0; // source: " << p.sourceVar << "\n";
        } else {
            out << "        plots_[\"" << p.title << "\"] = 0.0;\n";
        }
    }
    out << "    }\n\n";

    out << "    /// Get all computed plot values (read from bar data)\n"
        << "    std::map<std::string, double> plots() const { return plots_; }\n\n"
        << "    /// Check if indicator has enough bar data\n"
        << "    bool ready() const { return barIndex_ >= 2; }\n\n"
        << "    int barIndex() const { return barIndex_; }\n\n";

    // Strategy signal accessor
    if (!strategyCalls.empty()) {
        out << "    /// Get strategy signal: 0=hold, 1=entry_long, -1=entry_short, 2=exit_long, -2=exit_short, 3=close_all\n"
            << "    int signal() const { return signal_; }\n\n";
    }

    // Private members
    out << "private:\n";
    for (auto& ind : inds)
        out << "    " << ind.type << " " << ind.key << "_;\n";
    for (auto& m : macdInfos) {
        out << "    EMA " << m.key << "_fast_;\n"
            << "    EMA " << m.key << "_slow_;\n"
            << "    EMA " << m.key << "_sig_;\n"
            << "    double " << m.key << "_macd_{0.0};\n"
            << "    double " << m.key << "_signal_{0.0};\n"
            << "    double " << m.key << "_hist_{0.0};\n";
    }
    // Previous values for crossover/crossunder
    for (auto& cr : crossInfos) {
        out << "    double prev_" << cr.srcA << "_{0.0};\n"
            << "    double prev_" << cr.srcB << "_{0.0};\n";
    }
    if (needsVwap) {
        out << "    double cumPV_{0.0};\n"
            << "    double cumVol_{0.0};\n";
    }
    out << "    std::deque<double> closes_;\n";
    if (!dmiInfos.empty()) {
        out << "    std::deque<double> highs_;\n"
            << "    std::deque<double> lows_;\n";
    }
    out << "    std::map<std::string, double> plots_;\n"
        << "    int barIndex_{0};\n";
    if (!strategyCalls.empty()) {
        out << "    int signal_{0};\n";
    }
    out << "};\n\n} // namespace crypto\n";

    return out.str();
}

} // namespace crypto
