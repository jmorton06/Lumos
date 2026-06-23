#pragma once
#include "EditorPanel.h"
#include <imgui/Plugins/ImTextEditor.h>
#include <functional>
#include <vector>
#include <string>

namespace Lumos
{
    class TextEditPanel : public EditorPanel
    {
    public:
        struct Snippet
        {
            std::string Name;
            std::string Body;
        };
        struct SnippetGroup
        {
            std::string FileName;
            std::vector<Snippet> Snippets;
        };

        struct CompletionItem
        {
            std::string Display;     // shown in list (e.g. "GetRigidBody3DComponent")
            std::string Insert;      // what to type at cursor
            std::string Detail;      // shown faded right ("RigidBody3DComponent")
            std::string Signature;   // tooltip
            int         Kind = 0;
        };

        TextEditPanel(const std::string& filePath);
        ~TextEditPanel() = default;

        void OnImGui() override;
        void OnClose();

        void SetOnSaveCallback(const std::function<void()>& callback) { m_OnSaveCallback = callback; }
        void SetErrors(const std::unordered_map<int, std::string>& errors);

    private:
        void DrawSnippetBrowser();
        void LoadSnippets();

        // Autocomplete plumbing.
        void RebuildLocalsIndex();
        void UpdateCompletionContext();
        void BuildCompletionCandidates();
        void DrawCompletionPopup();
        void CommitCompletion(const CompletionItem& it);

        std::string m_FilePath;
        TextEditor editor;
        std::function<void()> m_OnSaveCallback;

        bool m_TextUnsaved = false;
        bool m_FocusMode = false;
        bool m_PreviousFocusMode = false;
        uint32_t m_SavedDockID = 0;

        float m_SavedTimer = -1.0f;
        bool m_KeyboardShowing = false;

        bool m_ShowSnippetBrowser = false;
        bool m_SnippetsLoaded = false;
        std::vector<SnippetGroup> m_SnippetGroups;
        int m_SelectedGroup = -1;
        int m_SelectedSnippet = -1;
        char m_SnippetFilter[128] = {};

        // ---- autocomplete state ----
        bool m_CompletionOpen = false;
        std::string                 m_CompletionPrefix;   // current word fragment
        std::string                 m_CompletionOwner;    // "" or receiver type ("Vec3", "RigidBody3D")
        bool                        m_CompletionIsMember = false; // true if triggered by . or :
        std::vector<CompletionItem> m_CompletionItems;
        int                         m_CompletionSelected = 0;
        TextEditor::Coordinates     m_CompletionAnchor;   // start of prefix in editor coords
        float                       m_CompletionPopupX = 0.0f;
        float                       m_CompletionPopupY = 0.0f;
        bool                        m_CompletionJustOpened = false;

        // var-name → type-name, rebuilt on demand from current document.
        std::vector<std::pair<std::string, std::string>> m_LocalsIndex;
        bool m_LocalsDirty = true;
    };
}
