#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <variant>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaError::FormulaError(Category category) 
    : category_(category)
{
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    if (category_ == Category::Ref) {
        return "#REF!"sv;
    }
    else if (category_ == Category::Value) {
        return "#VALUE!"sv;
    }
    else {
        return "#DIV/0!"sv;
    }
}

double ConvertToDouble(const std::string& text) {
    std::stringstream ss{ text };
    double result;
    ss >> result;
    if (ss.eof()) {
        return result;
    }
    throw FormulaError{ FormulaError::Category::Value };
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
        : ast_(ParseFormulaAST(expression))
    {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            auto compute_cell = [&sheet](const Position& pos) {
                auto cell = sheet.GetCell(pos);
                if (!cell) {
                    return 0.0;
                }
                auto cell_val = cell->GetValue();
                if (std::holds_alternative<double>(cell_val)) {
                    return std::get<double>(cell_val);
                }
                else if (std::holds_alternative<std::string>(cell_val)) {
                    auto text = std::get<std::string>(cell_val);
                    if (text.empty()) {
                        return 0.0;
                    }
                    return ConvertToDouble(text);
                }
                else {
                    throw std::get<FormulaError>(cell_val);
                }
            };
            return ast_.Execute(compute_cell);
        }
        catch (FormulaError const& e) {
            return e;
        }
        catch (InvalidPositionException const& e) {
            return FormulaError{ FormulaError::Category::Ref };
        }
    }

    std::string GetExpression() const override {
        std::stringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::forward_list<Position> cells = ast_.GetCells();
        std::set<Position> ref_cells_set{ cells.begin(), cells.end() };
        return { ref_cells_set.begin(), ref_cells_set.end() };
    }
private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException{ "" };
    }
}