using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Runtime::InteropServices;

public ref class RenderWindowHost : public HwndHost {
public:
    RenderWindowHost();
    ~RenderWindowHost();

    HandleRef GetHwnd();

protected:
    virtual HandleRef BuildWindowCore(HandleRef hwndParent) override;
    virtual void DestroyWindowCore(HandleRef hwnd) override;

private:
    HWND m_hwnd;

};
