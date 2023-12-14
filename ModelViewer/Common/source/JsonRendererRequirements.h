#pragma once

#include "base/RendererRequirements_Base.h"
#include <string>
#include <vector>
#include <optional>
#include <istream>

namespace Graphics {

class JsonRendererRequirementsImpl;

class JsonRendererRequirements : public RendererRequirements {
public:
    JsonRendererRequirements();
    virtual ~JsonRendererRequirements();

    void Initialize(std::string const &settingsFilePath);
    void Initialize(std::istream &dataStream);

    std::optional<std::string> GetString(char const *jsonQuery) const;
    std::optional<f64> GetNumber(char const *jsonQuery) const;
    std::optional<bool> GetBoolean(char const *jsonQuery) const;
    std::optional<std::vector<std::string>> GetArray(char const *jsonQuery) const;

private:
    JsonRendererRequirementsImpl *m_impl;
};

} // namespace Graphics
