#pragma once

#include <string>
#include <vector>
#include <optional>

namespace Graphics {

class WindowSurface;

class RendererRequirements {
public:
    virtual ~RendererRequirements() = 0;

    virtual std::optional<std::string> GetString(char const *query) const = 0;
    virtual std::optional<f64> GetNumber(char const *query) const = 0;
    virtual std::optional<bool> GetBoolean(char const *query) const = 0;
    virtual std::optional<std::vector<std::string>> GetArray(char const *query) const = 0;

    virtual WindowSurface *GetWindowSurface(int index) const = 0;
};

} // namespace Graphics
