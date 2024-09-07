#include "pch.h"
#include "JsonRendererRequirementsImpl.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/pointer.h"
#include <fstream>
#include <filesystem>

namespace Graphics {

JsonRendererRequirementsImpl::JsonRendererRequirementsImpl()
  : m_document(nullptr) {
}

JsonRendererRequirementsImpl::~JsonRendererRequirementsImpl() {
    if (m_document) {

    }
}

void JsonRendererRequirementsImpl::Initialize(std::string const &settingsFilePath) {
#ifdef _WIN32
    wchar_t cwd[MAX_PATH];
    auto ret = GetModuleFileName(NULL, cwd, MAX_PATH);
    ASSERT(ret != 0);
    std::filesystem::path exePath(cwd);
    exePath = exePath.parent_path();
#else
#error Not Supported
#endif
    std::ifstream jsonFile(exePath /= settingsFilePath);
    ASSERT_MSG(!jsonFile.fail() && jsonFile.is_open(), L"Invalid file path when parsing renderer requirements: %hs", settingsFilePath.c_str());
    Initialize(jsonFile);
    jsonFile.close();
}

void JsonRendererRequirementsImpl::Initialize(std::istream &dataStream) {
    rapidjson::IStreamWrapper dataStreamWrapper(dataStream);
    m_document = new rapidjson::Document;
    m_document->ParseStream(dataStreamWrapper);
    ASSERT_MSG(!m_document->HasParseError(), L"Invalid JSON stream when parsing renderer requirements");
}

void JsonRendererRequirementsImpl::AddWindowSurface(WindowSurface *surface) {
    m_surfaces.emplace_back(surface);
}

std::optional<f64> JsonRendererRequirementsImpl::GetNumber(char const *jsonQuery) const {
    ASSERT_MSG(m_document, L"No json data loaded");
    std::optional<f64> result;
    auto value = rapidjson::Pointer(jsonQuery).Get(*m_document);
    if (value) {
        ASSERT_MSG(value->IsNumber(), L"JSON value is not of type %hs (%hs)", "Number", value->GetString());// jsonQuery);
        result = value->GetDouble();
    }
    return result;
}

std::optional<bool> JsonRendererRequirementsImpl::GetBoolean(char const *jsonQuery) const {
    ASSERT_MSG(m_document, L"No json data loaded");
    std::optional<bool> result;
    auto value = rapidjson::Pointer(jsonQuery).Get(*m_document);
    if (value) {
        ASSERT_MSG(value->IsBool(), L"JSON value is not of type %hs (%hs)", "Boolean", jsonQuery);
        result = value->GetBool();
    }
    return result;
}

std::optional<std::string> JsonRendererRequirementsImpl::GetString(char const *jsonQuery) const {
    ASSERT_MSG(m_document, L"No json data loaded");
    std::optional<std::string> result;
    auto value = rapidjson::Pointer(jsonQuery).Get(*m_document);
    if (value) {
        ASSERT_MSG(value->IsString(), L"JSON value is not of type %hs (%hs)", "String", jsonQuery);
        result = std::string(value->GetString(), value->GetStringLength());
    }
    return result;
}

std::optional<std::vector<std::string>> JsonRendererRequirementsImpl::GetArray(char const *jsonQuery) const {
    ASSERT_MSG(m_document, L"No json data loaded");
    std::optional<std::vector<std::string>> result;
    auto value = rapidjson::Pointer(jsonQuery).Get(*m_document);
    if (value) {
        ASSERT_MSG(value->IsArray(), L"JSON value is not of type %hs (%hs)", "Array", jsonQuery);
        auto queryArray = value->GetArray();
        result = std::vector<std::string>();
        result->reserve(queryArray.Size());
        for (auto &it : queryArray) {
            result->emplace_back(std::string(it.GetString(), it.GetStringLength()));
        }
    }
    return result;
}

WindowSurface *JsonRendererRequirementsImpl::GetWindowSurface(int index) const {
    if (index < m_surfaces.size()) {
        return m_surfaces[index];
    }
    return nullptr;
}

} // namespace Graphics
