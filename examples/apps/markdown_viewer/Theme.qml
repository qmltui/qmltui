import QtQml 2.15

QtObject {
    id: theme

    property string activeTheme: "tokyo_night" // tokyo_night, dracula, nord, monokai, amber_crt

    // Background & Surfaces
    readonly property string bg: (activeTheme === "amber_crt") ? "black" :
                                 (activeTheme === "dracula") ? "#282a36" :
                                 (activeTheme === "nord") ? "#2e3440" :
                                 (activeTheme === "monokai") ? "#272822" : "#1a1b26"

    readonly property string surface: (activeTheme === "amber_crt") ? "#1a1000" :
                                     (activeTheme === "dracula") ? "#44475a" :
                                     (activeTheme === "nord") ? "#3b4252" :
                                     (activeTheme === "monokai") ? "#3e3d32" : "#24283b"

    readonly property string border: (activeTheme === "amber_crt") ? "#ffb000" :
                                    (activeTheme === "dracula") ? "#6272a4" :
                                    (activeTheme === "nord") ? "#4c566a" :
                                    (activeTheme === "monokai") ? "#75715e" : "#414868"

    // Accents & Text
    readonly property string textPrimary: (activeTheme === "amber_crt") ? "#ffcc00" :
                                          (activeTheme === "dracula") ? "#f8f8f2" :
                                          (activeTheme === "nord") ? "#eceff4" :
                                          (activeTheme === "monokai") ? "#f8f8f2" : "#c0caf5"

    readonly property string textMuted: (activeTheme === "amber_crt") ? "#996600" :
                                        (activeTheme === "dracula") ? "#6272a4" :
                                        (activeTheme === "nord") ? "#d8dee9" :
                                        (activeTheme === "monokai") ? "#75715e" : "#565f89"

    readonly property string accent: (activeTheme === "amber_crt") ? "#ffb000" :
                                     (activeTheme === "dracula") ? "#bd93f9" :
                                     (activeTheme === "nord") ? "#88c0d0" :
                                     (activeTheme === "monokai") ? "#a6e22e" : "#7aa2f7"

    readonly property string accentSecondary: (activeTheme === "amber_crt") ? "#ffd700" :
                                              (activeTheme === "dracula") ? "#ff79c6" :
                                              (activeTheme === "nord") ? "#81a1c1" :
                                              (activeTheme === "monokai") ? "#f92672" : "#bb9af7"

    readonly property string success: (activeTheme === "amber_crt") ? "#ffb000" :
                                      (activeTheme === "dracula") ? "#50fa7b" :
                                      (activeTheme === "nord") ? "#a3be8c" :
                                      (activeTheme === "monokai") ? "#a6e22e" : "#9ece6a"

    readonly property string warning: (activeTheme === "amber_crt") ? "#ff8800" :
                                      (activeTheme === "dracula") ? "#f1fa8c" :
                                      (activeTheme === "nord") ? "#ebcb8b" :
                                      (activeTheme === "monokai") ? "#e6db74" : "#e0af68"

    function cycleTheme() {
        if (activeTheme === "tokyo_night") activeTheme = "dracula"
        else if (activeTheme === "dracula") activeTheme = "nord"
        else if (activeTheme === "nord") activeTheme = "monokai"
        else if (activeTheme === "monokai") activeTheme = "amber_crt"
        else activeTheme = "tokyo_night"
    }
}
