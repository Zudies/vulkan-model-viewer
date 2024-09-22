#pragma once

public interface class GraphicsEngineToUIInterface {
    public:
        // Content ID is the Name of the UI element
        // Content value is the localization key as can be found in Localization.resx
        System::String ^GetEngineValue(System::String ^contentId);
        void SetEngineValue(System::String ^contentId, System::String ^contentValue);
};
