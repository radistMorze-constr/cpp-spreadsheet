#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>
#include <algorithm>
#include <vector>


class Cell::Impl {
public:
    using Value = std::variant<std::string, double, FormulaError>;

    explicit Impl(std::string text)
        : value_(text)
    {}

    virtual ~Impl() = default;

    virtual std::string GetText() const = 0;

    virtual Value GetValue(SheetInterface& sheet) const = 0;

    virtual std::vector<Position> GetReferencedCells() const = 0;

protected:
    std::string value_;
};

class Cell::EmptyImpl : public Impl {
public:
    explicit EmptyImpl()
        : Impl("")
    {}

    std::string GetText() const override {
        return value_;
    }

    Value GetValue(SheetInterface&) const override {
        return value_;
    }

    std::vector<Position> GetReferencedCells() const {
        return {};
    }
};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string& text)
        : Cell::Impl(std::move(text)) {}

    std::string GetText() const override {
        return value_;
    }

    Value GetValue(SheetInterface&) const override {
        if (value_[0] == ESCAPE_SIGN) {
            return value_.substr(1);
        }
        return value_;
    }

    std::vector<Position> GetReferencedCells() const {
        return {};
    }
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string text)
        : Impl(text)
        , formula_(ParseFormula(text))
    {}

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    Value GetValue(SheetInterface& sheet) const override {
        const auto& value = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }
        if (std::holds_alternative<FormulaError>(value)) {
            return std::get<FormulaError>(value);
        }
        return "";
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> formula_;
};


Cell::Cell(Sheet& sheet)
    : sheet_(sheet)
    , impl_(std::make_unique<EmptyImpl>())
{}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    using namespace std::literals;

    if (text == GetText()) {
        return;
    }
    std::unique_ptr<Impl> temp;
    if (text.empty()) {
        temp = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        temp = std::make_unique<FormulaImpl>(text.substr(1));
    }
    else {
        temp = std::make_unique<TextImpl>(text);
    }

    {
        std::unordered_set<Position, PositionHasher> verified_cells;
        ThrowIfCircularDependency(temp->GetReferencedCells(), this, verified_cells);
    }

    InvalidateCache();

    for (const auto& cell : referenced_cells_) {
        //auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos)));
        cell->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();

    for (const auto& pos : temp->GetReferencedCells()) {
        auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        referenced_cells_.insert(cell);
        if (!cell) {
            sheet_.SetCell(pos, ""s);
            cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        }
        cell->dependent_cells_.insert(this);
    }
    impl_ = std::move(temp);
}

void Cell::ThrowIfCircularDependency(const std::vector<Position>& cells, Cell* current_cell, std::unordered_set<Position, PositionHasher>& verified_cells) {
    for (const auto& pos : cells) {
        if (verified_cells.count(pos)) {
            continue;
        }
        if (sheet_.GetCell(pos) == current_cell) {
            throw CircularDependencyException("");
        }
        verified_cells.insert(pos);
        auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        if (!cell) {
            continue;
        }
        ThrowIfCircularDependency(cell->GetReferencedCells(), current_cell, verified_cells);
    }
}

void Cell::Clear() {
    Set("");
}

void Cell::InvalidateCache() {
    if (!cached_value_) {
        return;
    }
    cached_value_.reset();
    for (const auto& cell : dependent_cells_) {
        //auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        cell->InvalidateCache();
    }
}

Cell::Value Cell::GetValue() const {
    if (!cached_value_) {
        cached_value_ = impl_->GetValue(sheet_);
    }

    return *cached_value_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}