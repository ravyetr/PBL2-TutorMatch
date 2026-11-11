import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: tutorCardRoot
    color: Theme.cardBg
    radius: 10
    border.color: Theme.cardBorder
    border.width: 1

    property var tutorData: ({})

    signal detailRequested(var tutor)
    signal bookingRequested(var tutor)

    function formatCurrency(amount) {
        if (!amount || isNaN(amount)) return "0 VNĐ / buổi";
        return amount.toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ / buổi";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Top Row: Avatar & Name
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Circular Initials Avatar
            Rectangle {
                width: 44
                height: 44
                radius: 22
                color: Theme.sidebarBg
                Layout.alignment: Qt.AlignTop

                Text {
                    anchors.centerIn: parent
                    text: tutorCardRoot.tutorData.initials || "G"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textLight
                }
            }

            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: tutorCardRoot.tutorData.fullName || "Gia sư"
                        font.bold: true
                        font.pixelSize: 14
                        color: Theme.textDark
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // Rating & Reviews Summary
                RowLayout {
                    spacing: 6
                    Text {
                        text: (tutorCardRoot.tutorData.rating || 5.0).toFixed(1) + " ★"
                        font.bold: true
                        font.pixelSize: 12
                        color: "#d97706"
                    }
                    Text {
                        text: (tutorCardRoot.tutorData.totalReviews > 0) ? "(" + tutorCardRoot.tutorData.totalReviews + " đánh giá)" : "(Chưa có đánh giá)"
                        font.pixelSize: 11
                        color: Theme.textMuted
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // Subject Specialty Badge & Price Tag
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                height: 24
                Layout.fillWidth: true
                Layout.maximumWidth: Math.max(80, parent.width - priceText.implicitWidth - 20)
                implicitWidth: subjectText.implicitWidth + 16
                radius: 4
                color: Theme.accentBlue
                border.color: Theme.cardBorder
                border.width: 1

                Text {
                    id: subjectText
                    anchors.centerIn: parent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: tutorCardRoot.tutorData.subject || "Chuyên môn"
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textDark
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Item { Layout.fillWidth: true }

            // Price Tag with safe layout constraints and elision
            Text {
                id: priceText
                Layout.alignment: Qt.AlignRight
                text: tutorCardRoot.formatCurrency(tutorCardRoot.tutorData.hourlyRate)
                font.bold: true
                font.pixelSize: 13
                color: "#d97706"
                elide: Text.ElideRight
            }
        }

        // Bio snippet
        Text {
            Layout.fillWidth: true
            text: tutorCardRoot.tutorData.bio || "Gia sư uy tín, có nhiều năm kinh nghiệm giảng dạy."
            font.pixelSize: 11
            color: Theme.textMuted
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.cardBorder
        }

        // Bottom Action Buttons with Layout.fillWidth & zero minimum widths to prevent overflow
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Xem chi tiết"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 0
                height: 34
                contentItem: Text {
                    text: parent.text
                    font.bold: true
                    font.pixelSize: 11
                    color: Theme.textDark
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                    radius: 6
                    border.color: Theme.cardBorder
                }
                onClicked: tutorCardRoot.detailRequested(tutorCardRoot.tutorData)
            }

            Button {
                text: "Đăng ký học"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 0
                height: 34
                contentItem: Text {
                    text: parent.text
                    font.bold: true
                    font.pixelSize: 11
                    color: Theme.btnText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    color: parent.hovered ? Theme.sidebarBg : Theme.btnPrimary
                    radius: 6
                }
                onClicked: tutorCardRoot.bookingRequested(tutorCardRoot.tutorData)
            }
        }
    }
}
