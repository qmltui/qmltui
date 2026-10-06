import QtQml

QtObject {
    id: theme

    property string activeTheme: "aurora" // aurora, tokyo_night, dracula, nord, monokai, amber_crt

    // Background & Surfaces
    readonly property string bg: (activeTheme === "aurora" || activeTheme === "modern") ? "#181824" :
                                 (activeTheme === "amber_crt") ? "black" :
                                 (activeTheme === "dracula") ? "#282a36" :
                                 (activeTheme === "nord") ? "#2e3440" :
                                 (activeTheme === "monokai") ? "#272822" : "#1a1b26"

    readonly property string surface: (activeTheme === "aurora" || activeTheme === "modern") ? "#202030" :
                                     (activeTheme === "amber_crt") ? "#1a1000" :
                                     (activeTheme === "dracula") ? "#44475a" :
                                     (activeTheme === "nord") ? "#3b4252" :
                                     (activeTheme === "monokai") ? "#3e3d32" : "#24283b"

    readonly property string border: (activeTheme === "aurora" || activeTheme === "modern") ? "#414558" :
                                    (activeTheme === "amber_crt") ? "#ffb000" :
                                    (activeTheme === "dracula") ? "#6272a4" :
                                    (activeTheme === "nord") ? "#4c566a" :
                                    (activeTheme === "monokai") ? "#75715e" : "#414868"

    // Accents & Text
    readonly property string textPrimary: (activeTheme === "aurora" || activeTheme === "modern") ? "#e2e8f0" :
                                          (activeTheme === "amber_crt") ? "#ffcc00" :
                                          (activeTheme === "dracula") ? "#f8f8f2" :
                                          (activeTheme === "nord") ? "#eceff4" :
                                          (activeTheme === "monokai") ? "#f8f8f2" : "#c0caf5"

    readonly property string textMuted: (activeTheme === "aurora" || activeTheme === "modern") ? "#626880" :
                                        (activeTheme === "amber_crt") ? "#996600" :
                                        (activeTheme === "dracula") ? "#6272a4" :
                                        (activeTheme === "nord") ? "#d8dee9" :
                                        (activeTheme === "monokai") ? "#75715e" : "#565f89"

    readonly property string accent: (activeTheme === "aurora" || activeTheme === "modern") ? "#00d7ff" :
                                     (activeTheme === "amber_crt") ? "#ffb000" :
                                     (activeTheme === "dracula") ? "#bd93f9" :
                                     (activeTheme === "nord") ? "#88c0d0" :
                                     (activeTheme === "monokai") ? "#a6e22e" : "#7aa2f7"

    readonly property string accentSecondary: (activeTheme === "aurora" || activeTheme === "modern") ? "#ff79c6" :
                                              (activeTheme === "amber_crt") ? "#ffd700" :
                                              (activeTheme === "dracula") ? "#ff79c6" :
                                              (activeTheme === "nord") ? "#81a1c1" :
                                              (activeTheme === "monokai") ? "#f92672" : "#bb9af7"

    readonly property string success: (activeTheme === "aurora" || activeTheme === "modern") ? "#50fa7b" :
                                      (activeTheme === "amber_crt") ? "#ffb000" :
                                      (activeTheme === "dracula") ? "#50fa7b" :
                                      (activeTheme === "nord") ? "#a3be8c" :
                                      (activeTheme === "monokai") ? "#a6e22e" : "#9ece6a"

    readonly property string warning: (activeTheme === "aurora" || activeTheme === "modern") ? "#ffb86c" :
                                      (activeTheme === "amber_crt") ? "#ff8800" :
                                      (activeTheme === "dracula") ? "#f1fa8c" :
                                      (activeTheme === "nord") ? "#ebcb8b" :
                                      (activeTheme === "monokai") ? "#e6db74" : "#e0af68"

    // Modern badge & semantic tokens
    readonly property string h1Bg: (activeTheme === "aurora" || activeTheme === "modern") ? "#5856d6" :
                                   (activeTheme === "amber_crt") ? "#ffb000" :
                                   (activeTheme === "dracula") ? "#bd93f9" :
                                   (activeTheme === "nord") ? "#5e81ac" :
                                   (activeTheme === "monokai") ? "#f92672" : "#7aa2f7"

    readonly property string h1Fg: (activeTheme === "aurora" || activeTheme === "modern") ? "#ffffff" :
                                   (activeTheme === "amber_crt") ? "#000000" :
                                   (activeTheme === "dracula") ? "#282a36" :
                                   (activeTheme === "nord") ? "#eceff4" :
                                   (activeTheme === "monokai") ? "#272822" : "#ffffff"

    readonly property string inlineCodeFg: (activeTheme === "aurora" || activeTheme === "modern") ? "#f43f5e" :
                                           (activeTheme === "amber_crt") ? "#ffcc00" :
                                           (activeTheme === "dracula") ? "#ff79c6" :
                                           (activeTheme === "nord") ? "#88c0d0" :
                                           (activeTheme === "monokai") ? "#f92672" : "#e0af68"

    readonly property string inlineCodeBg: (activeTheme === "aurora" || activeTheme === "modern") ? "#4c1d24" :
                                           (activeTheme === "amber_crt") ? "#2a1800" :
                                           (activeTheme === "dracula") ? "#44475a" :
                                           (activeTheme === "nord") ? "#3b4252" :
                                           (activeTheme === "monokai") ? "#3e3d32" : "#24283b"

    readonly property string syntaxFunction: (activeTheme === "aurora" || activeTheme === "modern") ? "#50fa7b" :
                                             (activeTheme === "amber_crt") ? "#ffd700" :
                                             (activeTheme === "dracula") ? "#50fa7b" :
                                             (activeTheme === "nord") ? "#88c0d0" :
                                             (activeTheme === "monokai") ? "#a6e22e" : "#7aa2f7"

    readonly property string syntaxKeyword: (activeTheme === "aurora" || activeTheme === "modern") ? "#ff79c6" : accentSecondary
    readonly property string syntaxType: (activeTheme === "aurora" || activeTheme === "modern") ? "#00d7ff" : accent
    readonly property string syntaxString: (activeTheme === "aurora" || activeTheme === "modern") ? "#ffb86c" : success
    readonly property string syntaxComment: textMuted
    readonly property string syntaxNumber: (activeTheme === "aurora" || activeTheme === "modern") ? "#bd93f9" : warning
    readonly property string syntaxPreprocessor: (activeTheme === "aurora" || activeTheme === "modern") ? "#ff79c6" : warning
    readonly property string syntaxSymbol: (activeTheme === "aurora" || activeTheme === "modern") ? "#ff79c6" : accentSecondary

    function cycleTheme() {
        if (activeTheme === "aurora" || activeTheme === "modern") activeTheme = "tokyo_night"
        else if (activeTheme === "tokyo_night") activeTheme = "dracula"
        else if (activeTheme === "dracula") activeTheme = "nord"
        else if (activeTheme === "nord") activeTheme = "monokai"
        else if (activeTheme === "monokai") activeTheme = "amber_crt"
        else activeTheme = "aurora"
    }
}
