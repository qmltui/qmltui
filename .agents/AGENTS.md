# Qtui QML Key Handling
- The underlying `Tui` engine does NOT automatically handle the "Q" or "Escape" keys to quit applications.
- Whenever writing or updating a `Main.qml` file (or any root `Terminal` component) for a Qtui application/example, you MUST explicitly handle quitting by adding a `Keys.onPressed` block.
- Example implementation:
  ```qml
  Keys.onPressed: (event) => {
      if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
          Qt.quit()
          event.accepted = true
      }
  }
  ```
