#pragma once

namespace Lumos
{
    struct UIDockPanelContext;

    // "Support Lumos" panel: consumable tips through the App Store where there is one,
    // else a link to GitHub Sponsors. The tips are set up in App Store Connect under kTipIds.
    namespace TipsPanel
    {
        void Draw(const UIDockPanelContext& context);
        // Store events land here every frame, so a purchase finishing with the panel shut still thanks
        void Update();
    }
}
