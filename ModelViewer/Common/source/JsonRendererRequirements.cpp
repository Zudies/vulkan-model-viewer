#include "pch.h"
#include "JsonRendererRequirements.h"
#include "JsonRendererRequirementsImpl.h"

namespace Graphics {

JsonRendererRequirements::JsonRendererRequirements()
  : m_impl(nullptr) {
}

JsonRendererRequirements::~JsonRendererRequirements() {
    delete m_impl;
}

void JsonRendererRequirements::Initialize(std::string const &settingsFilePath) {
    ASSERT(!m_impl);
    m_impl = new JsonRendererRequirementsImpl();
    m_impl->Initialize(settingsFilePath);
}

void JsonRendererRequirements::Initialize(std::istream &dataStream) {
    ASSERT(!m_impl);
    m_impl = new JsonRendererRequirementsImpl();
    m_impl->Initialize(dataStream);
}

#define JSON_REQUIREMENTS_GETTER_IMPL(funcName, type) \
    std::optional<type> JsonRendererRequirements::Get##funcName(char const *jsonQuery) const { \
        ASSERT(m_impl); \
        return m_impl->Get##funcName(jsonQuery); \
    }

JSON_REQUIREMENTS_GETTER_IMPL(String, std::string)
JSON_REQUIREMENTS_GETTER_IMPL(Number, f64)
JSON_REQUIREMENTS_GETTER_IMPL(Boolean, bool)
JSON_REQUIREMENTS_GETTER_IMPL(Array, std::vector<std::string>)

} // namespace Graphics
