import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: reviewModal
    anchors.fill: parent
    visible: false
    z: 999

    property var bookingData: ({})
    property int selectedRating: 5
    property string errorMessage: ""

    signal reviewSubmitted()

    function openWithBooking(booking) {
        bookingData = booking || {}
        selectedRating = 5
        errorMessage = ""
        commentInput.text = ""
        reviewModal.visible = true
    }

    // Modal Background Overlay Dimming
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        MouseArea {
            anchors.fill: parent
            onClicked: {} // Prevent click-through
        }
    }

    // Centered Modal Window Card
    Rectangle {
        width: 520
        height: 480
        anchors.centerIn: parent
        radius: 12
        color: Theme.cardBg
        border.color: Theme.cardBorder
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            // Header
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "ĐÁNH GIÁ GIA SƯ & BUỔI HỌC"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "✕"
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        color: Theme.textMuted
                    }
                    background: Rectangle { color: "transparent" }
                    onClicked: reviewModal.visible = false
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.cardBorder
            }

            // Tutor & Course Summary Header Box
            Rectangle {
                Layout.fillWidth: true
                height: 50
                color: Theme.bgApp
                radius: 6
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Text {
                        text: "Gia sư: " + (bookingData.tutorName || "")
                        font.bold: true
                        font.pixelSize: 14
                        color: Theme.textDark
                    }
                    Text {
                        text: " |  Môn: " + (bookingData.subject || "")
                        font.pixelSize: 13
                        font.bold: true
                        color: "#d97706"
                    }
                }
            }

            // Error Banner
            Rectangle {
                Layout.fillWidth: true
                height: 32
                radius: 6
                color: "#fff1f0"
                border.color: "#ff4d4f"
                border.width: 1
                visible: reviewModal.errorMessage !== ""

                Text {
                    anchors.centerIn: parent
                    text: reviewModal.errorMessage
                    font.pixelSize: 11
                    font.bold: true
                    color: "#cf1322"
                }
            }

            Text {
                text: "Bạn đánh giá mức độ hài lòng về gia sư (1 - 5 sao):"
                font.bold: true
                font.pixelSize: 12
                color: Theme.textDark
            }

            // 5-Star Rating Selector
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8

                Repeater {
                    model: 5
                    Rectangle {
                        width: 44
                        height: 44
                        radius: 8
                        color: (index + 1 <= reviewModal.selectedRating) ? Theme.cMistyRose : Theme.bgApp
                        border.color: (index + 1 <= reviewModal.selectedRating) ? "#d97706" : Theme.cardBorder
                        border.width: (index + 1 <= reviewModal.selectedRating) ? 2 : 1

                        Text {
                            anchors.centerIn: parent
                            text: (index + 1 <= reviewModal.selectedRating) ? "★" : "☆"
                            font.pixelSize: 22
                            color: (index + 1 <= reviewModal.selectedRating) ? "#d97706" : Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                reviewModal.selectedRating = index + 1
                            }
                        }
                    }
                }
            }

            Text {
                text: "Nhận xét & Phản hồi chi tiết (*):"
                font.bold: true
                font.pixelSize: 12
                color: Theme.textDark
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: commentInput
                    placeholderText: "Nhập nhận xét của bạn về phương pháp dạy, thái độ và hiệu quả buổi học..."
                    color: Theme.textDark
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    background: Rectangle {
                        color: Theme.bgApp
                        radius: 6
                        border.color: commentInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                    }
                }
            }

            // Footer Actions
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    text: "Hủy bỏ"
                    Layout.fillWidth: true
                    height: 40
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: Theme.textDark
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                        radius: 6
                        border.color: Theme.cardBorder
                    }
                    onClicked: reviewModal.visible = false
                }

                Button {
                    text: "Gửi đánh giá"
                    Layout.fillWidth: true
                    height: 40
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: Theme.btnText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.sidebarBg : Theme.btnPrimary
                        radius: 6
                    }
                    onClicked: {
                        var comment = commentInput.text.trim();
                        if (comment.length === 0) {
                            reviewModal.errorMessage = "Vui lòng nhập nội dung nhận xét!";
                            return;
                        }

                        if (typeof guiBridge !== "undefined") {
                            var ok = guiBridge.qmlSubmitReview(reviewModal.bookingData.id, reviewModal.selectedRating, comment);
                            if (ok) {
                                reviewModal.visible = false;
                                reviewModal.reviewSubmitted();
                            } else {
                                reviewModal.errorMessage = guiBridge.statusMessage || "Lỗi gửi đánh giá!";
                            }
                        } else {
                            reviewModal.visible = false;
                            reviewModal.reviewSubmitted();
                        }
                    }
                }
            }
        }
    }
}
