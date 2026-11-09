pragma Singleton
import QtQuick

QtObject {
    // Primary Brand Colors
    readonly property color cBurgundy:       "#6C0820"
    readonly property color cCherryBlossom:  "#F2AEBC"
    readonly property color cMistyRose:      "#F2DCDB"
    readonly property color cSilverLakeBlue: "#5A86CB"
    readonly property color cLapisLazuli:    "#3D5D91"

    // Semantic Surface & Layout Tokens
    readonly property color bgApp:             "#FAF4F4"
    readonly property color sidebarBg:         "#3D5D91"
    readonly property color sidebarActive:     "#F2AEBC"
    readonly property color sidebarActiveText: "#6C0820"

    readonly property color cardBg:            "#FFFFFF"
    readonly property color cardBorder:        "#F2DCDB"

    // Text & Typography Tokens
    readonly property color textDark:          "#3D2B2F"
    readonly property color textHeading:       "#6C0820"
    readonly property color textMuted:         "#6E7A8A"
    readonly property color textLight:         "#FFFFFF"

    // Action & Button Tokens
    readonly property color btnPrimary:        "#6C0820"
    readonly property color btnPrimaryHover:   "#8E1432"
    readonly property color btnText:           "#FFFFFF"

    // Timetable & Slot Tokens
    readonly property color slotBookedBg:      "#F2AEBC"
    readonly property color slotBookedText:    "#6C0820"
    readonly property color slotBusyBg:        "#F2DCDB"
    readonly property color slotFreeBorder:    "#5A86CB"

    // Legacy / Backward Compatibility Tokens
    readonly property color accentBlue:        "#5A86CB"
}
