import QtQuick
import Library

// Caption above a form input — shared by FormField and the genre picker so the
// two read as one column of fields.
Text {
    color: Theme.textPrimary
    font.pixelSize: Styles.fontSize.bodySmall
    font.weight: Styles.fontWeight.semibold
    elide: Text.ElideRight
}
