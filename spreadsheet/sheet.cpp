#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <utility>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (!cells_.count(pos)) {
        cells_.emplace(pos, *this);
    }
    cells_.at(pos).Set(pos, std::move(text));
    if (pos.col + 1 > size_.cols) {
        size_.cols = pos.col + 1;
    }
    if (pos.row + 1 > size_.rows) {
        size_.rows = pos.row + 1;
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (!IsInsideSheet(pos)) {
        return nullptr;
    }
    if (!cells_.count(pos)) {
        return nullptr;
    }
    return &cells_.at(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (!cells_.count(pos)) {
        return;
    }
    SetCell(pos, ""s);
    cells_.erase(pos);
    if (pos.row == size_.rows - 1) {
        auto row_max = std::max_element(cells_.begin(), cells_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first.row < rhs.first.row;
            });
        if (row_max != cells_.end()) {
            size_.rows = row_max->first.row + 1;
        }
        else {
            size_.rows = 0;
        }
    }
    if (pos.col == size_.cols - 1) {
        auto col_max = std::max_element(cells_.begin(), cells_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first.col < rhs.first.col;
            });
        if (col_max != cells_.end()) {
            size_.cols = col_max->first.col + 1;
        }
        else {
            size_.cols = 0;
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::Print(std::ostream& output, Sheet::PrintType type) const {
    for (int i = 0; i < size_.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < size_.cols; ++j) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            auto pos = Position{ i, j };
            if (cells_.count(pos)) {
                if (type == PrintType::Text) {
                    output << cells_.at(pos).GetText();
                }
                else {
                    auto cell = cells_.at(pos).GetValue();
                    std::visit([&cell, &output](const auto& arg) {output << arg; }, cell);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    Print(output, PrintType::Values);
}

void Sheet::PrintTexts(std::ostream& output) const {
    Print(output, PrintType::Text);
}

bool Sheet::IsInsideSheet(Position pos) {
    return size_.rows >= pos.row && size_.cols >= pos.col;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}