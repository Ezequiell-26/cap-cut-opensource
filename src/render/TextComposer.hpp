#pragma once
#include "text/TextLayer.hpp"
#include <QString>

namespace ccos::render {
class TextComposer {
public:
    static QString apply(const QString& inputLabel,
                         const ccos::text::TextLayer& layer,
                         const QString& outputLabel);
};
}
