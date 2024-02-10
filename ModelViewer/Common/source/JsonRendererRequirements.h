#pragma once

#include "base/RendererRequirements_Base.h"
#include <istream>

namespace Graphics {

class JsonRendererRequirementsImpl;

class JsonRendererRequirements : public RendererRequirements {
public:
    JsonRendererRequirements();
    virtual ~JsonRendererRequirements();

    void Initialize(std::string const &settingsFilePath);
    void Initialize(std::istream &dataStream);
    void AddWindowSurface(WindowSurface *surface);

    virtual std::optional<std::string> GetString(char const *jsonQuery) const override;
    virtual std::optional<f64> GetNumber(char const *jsonQuery) const override;
    virtual std::optional<bool> GetBoolean(char const *jsonQuery) const override;
    virtual std::optional<std::vector<std::string>> GetArray(char const *jsonQuery) const override;

    virtual WindowSurface *GetWindowSurface(int index) const override;

private:
    JsonRendererRequirementsImpl *m_impl;
};

} // namespace Graphics
