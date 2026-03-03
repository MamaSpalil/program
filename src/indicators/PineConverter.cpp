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
        auto stmt = parseStmt();
        if (stmt.kind == PineStmt::Kind::INDICATOR) {
            script.name = stmt.name;
            if (stmt.expr && stmt.expr->boolVal) script.overlay = true;
        }
        script.statements.push_back(std::move(stmt));
        skipNL();
    }
    return script;
}

PineStmt PineConverter::Parser::parseStmt() {
    PineStmt s;

    // var declaration
    if (check(PineTok::KW_VAR)) {
        advance();
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

    // indicator(...)
    if (check(PineTok::IDENT) && cur().value == "indicator") {
        advance();
        expect(PineTok::LPAREN);
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

    // Assignment: ident = expr  OR  ident := expr
    if (check(PineTok::IDENT)) {
        size_t save = pos_;
        std::string name = cur().value;
        advance();
        if (match(PineTok::ASSIGN) || match(PineTok::REASSIGN)) {
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
            if (n == "close")  return cur_.close;
            if (n == "open")   return cur_.open;
            if (n == "high")   return cur_.high;
            if (n == "low")    return cur_.low;
            if (n == "volume") return cur_.volume;
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
    if (fn == "input.int" || fn == "input.float" || fn == "input.source") {
        return args.size() >= 1 ? args[0] : 0.0;
    }

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
    out << "// Auto-generated from Pine Script v" << script.version << ": "
        << script.name << "\n"
        << "#pragma once\n"
        << "#include \"EMA.h\"\n"
        << "#include \"RSI.h\"\n"
        << "#include \"ATR.h\"\n"
        << "#include \"PineScriptIndicator.h\"\n"
        << "#include <map>\n#include <string>\n#include <cmath>\n\n"
        << "namespace crypto {\n\n"
        << "class " << className << " {\n"
        << "public:\n";

    // Collect indicator instances needed
    struct IndInfo { std::string type; std::string key; int period; };
    std::vector<IndInfo> inds;
    std::vector<std::string> plotNames;

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
                if (fn == "ta.rsi") inds.push_back({"RSI", stmt.name, period});
                if (fn == "ta.atr") inds.push_back({"ATR", stmt.name, period});
            }
        }
        if (stmt.kind == PineStmt::Kind::PLOT) plotNames.push_back(stmt.plotTitle);
    }

    // Constructor
    out << "    " << className << "()\n        : ";
    for (size_t i = 0; i < inds.size(); ++i) {
        out << inds[i].key << "_(" << inds[i].period << ")";
        if (i + 1 < inds.size()) out << ", ";
    }
    out << " {}\n\n";

    // Update method
    out << "    void update(const Candle& c) {\n";
    for (auto& ind : inds) {
        if (ind.type == "ATR")
            out << "        " << ind.key << "_.update(c);\n";
        else
            out << "        " << ind.key << "_.updateValue(c.close);\n";
    }
    out << "\n";
    for (auto& ind : inds)
        out << "        double " << ind.key << " = " << ind.key << "_.value();\n";

    out << "\n        plots_.clear();\n";
    for (auto& p : plotNames)
        out << "        plots_[\"" << p << "\"] = 0.0; // wire to computed value\n";
    out << "    }\n\n";

    out << "    std::map<std::string, double> plots() const { return plots_; }\n\n";

    // Private members
    out << "private:\n";
    for (auto& ind : inds)
        out << "    " << ind.type << " " << ind.key << "_;\n";
    out << "    std::map<std::string, double> plots_;\n";
    out << "};\n\n} // namespace crypto\n";

    return out.str();
}

} // namespace crypto
