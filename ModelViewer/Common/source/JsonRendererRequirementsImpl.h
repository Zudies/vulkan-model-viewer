#pragma once

#include "rapidjson/document.h"
#include <string>
#include <vector>
#include <optional>
#include <istream>

namespace Graphics {
class WindowSurface;

class JsonRendererRequirementsImpl {
public:
    JsonRendererRequirementsImpl();
    ~JsonRendererRequirementsImpl();

    void Initialize(std::string const &settingsFilePath);
    void Initialize(std::istream &dataStream);
    void AddWindowSurface(WindowSurface *surface);

    std::optional<std::string> GetString(char const *jsonQuery) const;
    std::optional<f64> GetNumber(char const *jsonQuery) const;
    std::optional<bool> GetBoolean(char const *jsonQuery) const;
    std::optional<std::vector<std::string>> GetArray(char const *jsonQuery) const;

    WindowSurface *GetWindowSurface(int index) const;

private:
    rapidjson::Document *m_document;

    typedef std::vector<WindowSurface*> SurfaceList;
    SurfaceList m_surfaces;
};

} // namespace Graphics
