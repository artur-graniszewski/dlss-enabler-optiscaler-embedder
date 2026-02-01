#pragma once
#include <windows.h>

// Minimalna konfiguracja — można rozszerzyć w przyszłości
struct OptiScalerConfig {
    // Jeśli true (domyślnie), podczas Init zrobimy krótkie "spoofowanie" nazwy
    // własnego modułu na "dlss-enabler-upscaler.dll", żeby CheckWorkingMode()
    // w oryginalnym DllMain rozpoznał integrację z Enablerem.
    bool spoof_as_enabler = true;
};

// Inicjalizacja/zwinięcie w stylu FakeNVAPI
bool OptiScaler_Init(HMODULE self, const OptiScalerConfig* cfg);
void OptiScaler_Shutdown();
