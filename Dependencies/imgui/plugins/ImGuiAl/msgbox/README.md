# ImGuiAl::MsgBox

A modal message box dialog, similar to Microsoft Window's MesageBox.

## Usage

To use, include `imguial_msg.h` and declare a `ImGuiAl::MsgBox` variable somewhere, initialize it with the desired options, and then call `Draw` right after the control that makes the message box popup.

```C++
// Initialization
static const char* buttons[] = { "Abort", "Cancel", "Please No", NULL };
msgbox.Init( "Clear log?", ICON_MD_WARNING, "Are you really really sure you want to delete the entire log history and loose all that information forever?", buttons, true );

// Drawing
if ( ImGui::Button( "Clear log" ) )
{
  ImGui::OpenPopup( "Clear log?" );
}

int selected = msgbox.Draw();

switch ( selected )
{
case 0: // No button pressed
case 1: // First button pressed and so forth
case 2: // Second button pressed, and so forth...
}
```

If you pass `NULL` for the `icon` parameter, the dialog won't have any icon and the text will ocupy the entire width of the dialog.

The last parameter to `Init` is a boolean that controls if a checkbox with the text "Don't ask me again" will be rendered in the dialog. If the checkbox is rendered, and the user checks it before clicking on a button, the message box won't be rendered anymore, and will always return the index of the last button clicked when activated. If you want to show the dialog again, call its `AskAgain` method.
