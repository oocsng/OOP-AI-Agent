#include "calculator_tool.h"
#include <cmath>
#include <sstream>
#include <stack>
#include <cctype>
#include <stdexcept>

namespace agent {

class ExprParser {
private:
    std::string expr_;
    size_t pos_{0};

    char peek() const {
        while (pos_ < expr_.length() && std::isspace(expr_[pos_])) {
            const_cast<ExprParser*>(this)->pos_++;
        }
        if (pos_ >= expr_.length()) return '\0';
        return expr_[pos_];
    }

    char get() {
        char c = peek();
        if (c != '\0') pos_++;
        return c;
    }

    double parse_number() {
        size_t start = pos_;
        bool has_dot = false;
        while (pos_ < expr_.length() && (std::isdigit(expr_[pos_]) || expr_[pos_] == '.')) {
            if (expr_[pos_] == '.') {
                if (has_dot) break;
                has_dot = true;
            }
            pos_++;
        }
        if (start == pos_) {
            throw std::runtime_error("Expected number at position " + std::to_string(pos_));
        }
        return std::stod(expr_.substr(start, pos_ - start));
    }

    double parse_factor() {
        char c = peek();
        if (c == '-') {
            get();
            return -parse_factor();
        }
        if (c == '+') {
            get();
            return parse_factor();
        }
        if (c == '(') {
            get();
            double val = parse_expression();
            if (get() != ')') {
                throw std::runtime_error("Missing closing parenthesis");
            }
            return val;
        }
        return parse_number();
    }

    double parse_power() {
        double val = parse_factor();
        while (peek() == '^') {
            get();
            double right = parse_factor();
            val = std::pow(val, right);
        }
        return val;
    }

    double parse_term() {
        double val = parse_power();
        while (true) {
            char op = peek();
            if (op == '*') {
                get();
                val *= parse_power();
            } else if (op == '/') {
                get();
                double denom = parse_power();
                if (denom == 0.0) {
                    throw std::runtime_error("Division by zero");
                }
                val /= denom;
            } else {
                break;
            }
        }
        return val;
    }

    double parse_expression() {
        double val = parse_term();
        while (true) {
            char op = peek();
            if (op == '+') {
                get();
                val += parse_term();
            } else if (op == '-') {
                get();
                val -= parse_term();
            } else {
                break;
            }
        }
        return val;
    }

public:
    explicit ExprParser(std::string expr) : expr_(std::move(expr)) {}

    double evaluate() {
        double result = parse_expression();
        if (peek() != '\0') {
            throw std::runtime_error("Unexpected characters at end of expression");
        }
        return result;
    }
};

std::string CalculatorTool::execute(const nlohmann::json& args) {
    std::string expr;
    if (args.contains("expression") && args["expression"].is_string()) {
        expr = args["expression"].get<std::string>();
    } else if (args.contains("expr") && args["expr"].is_string()) {
        expr = args["expr"].get<std::string>();
    } else if (args.contains("raw_input") && args["raw_input"].is_string()) {
        expr = args["raw_input"].get<std::string>();
    } else if (args.is_string()) {
        expr = args.get<std::string>();
    } else {
        return "Error: Missing string argument 'expression'.";
    }

    try {
        ExprParser parser(expr);
        double result = parser.evaluate();
        if (std::floor(result) == result && !std::isinf(result) && std::abs(result) < 1e15) {
            return std::to_string(static_cast<long long>(result));
        } else {
            std::ostringstream ss;
            ss << result;
            return ss.str();
        }
    } catch (const std::exception& e) {
        return "Error evaluating expression: " + std::string(e.what());
    }
}

} // namespace agent
