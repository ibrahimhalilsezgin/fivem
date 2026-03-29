#pragma once

namespace Gui {
    // GUI thread'ini başlatır ve pencereyi oluşturur
    void Initialize();
    
    // Pencereyi kapatır ve GUI thread'ini sonlandırır
    void Shutdown();
}
