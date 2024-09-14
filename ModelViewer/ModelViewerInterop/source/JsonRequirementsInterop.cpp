#include "pch.h"
#include "JsonRequirementsInterop.h"
#include "JsonRendererRequirements.h"


JsonRequirements::JsonRequirements()
  : m_nativeRequirements(nullptr) {

}

JsonRequirements::~JsonRequirements() {
    delete m_nativeRequirements;
}

void JsonRequirements::Initialize(System::String ^settingsFile) {
    m_nativeRequirements = new Graphics::JsonRendererRequirements;

    std::string nativePath = msclr::interop::marshal_as<std::string>(settingsFile);
    m_nativeRequirements->Initialize(nativePath);
}

void JsonRequirements::AddWindowSurface(WindowSurfaceInterface ^window) {
    m_surfaces.Add(window); // Need to hold reference because managed surfaces own the native surface allocation
    m_nativeRequirements->AddWindowSurface(window->GetNativeSurface());
}

Graphics::RendererRequirements *JsonRequirements::GetNativeRequirements() {
    return m_nativeRequirements;
}
