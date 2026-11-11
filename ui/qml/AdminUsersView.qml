import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Rectangle {
    id: adminUsersView
    color: Theme.cardBg
    radius: 10
    border.color: Theme.cardBorder
    border.width: 1

    property int activeFilterIndex: 0
    property string currentFilterKey: "ALL"
    property string searchKeyword: ""
    property var usersModel: []

    function applyFilter(index) {
        var filterKeys = ["ALL", "STUDENT", "TUTOR", "LOCKED"]
        if (index >= 0 && index < filterKeys.length) {
            currentFilterKey = filterKeys[index]
        } else {
            currentFilterKey = "ALL"
        }
        reloadUsers()
    }

    function reloadUsers() {
        if (typeof guiBridge !== "undefined") {
            usersModel = guiBridge.qmlGetAdminUsers(currentFilterKey, searchKeyword)
        }
    }

    Component.onCompleted: {
        reloadUsers()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() {
            reloadUsers()
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 14

            // 1. Title Header & Reload Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "Quản lý tài khoản người dùng"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                    }
                    Text {
                        text: "Kiểm soát người dùng, phân quyền và tra cứu thông tin chi tiết"
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Tổng số: " + adminUsersView.usersModel.length + " người dùng"
                    font.bold: true
                    font.pixelSize: 13
                    color: Theme.textDark
                }

                Button {
                    text: "Làm mới"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textLight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 100
                        implicitHeight: 34
                        radius: 6
                        color: parent.hovered ? Theme.cSilverLakeBlue : Theme.sidebarBg
                    }
                    onClicked: reloadUsers()
                }
            }

            // Status Banner Notification
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 6
                color: Theme.bgApp
                border.color: Theme.accentBlue
                border.width: 1
                visible: (typeof guiBridge !== "undefined" && guiBridge.statusMessage !== "")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Text {
                        text: "Thông báo: " + ((typeof guiBridge !== "undefined") ? guiBridge.statusMessage : "")
                        font.pixelSize: 12
                        font.bold: true
                        color: Theme.textDark
                    }
                }
            }

            // 2. Search Bar Field
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 8
                color: Theme.bgApp
                border.color: searchInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: "🔍"
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }

                    TextField {
                        id: searchInput
                        Layout.fillWidth: true
                        placeholderText: "Tìm kiếm theo tên, username, email hoặc môn học..."
                        color: Theme.textDark
                        background: Rectangle { color: "transparent" }
                        onTextChanged: {
                            adminUsersView.searchKeyword = text
                            adminUsersView.reloadUsers()
                        }
                    }

                    Button {
                        visible: searchInput.text !== ""
                        text: "✕"
                        contentItem: Text { text: parent.text; font.pixelSize: 12; color: Theme.textMuted }
                        background: Rectangle { color: "transparent" }
                        onClicked: {
                            searchInput.text = ""
                            adminUsersView.searchKeyword = ""
                            adminUsersView.reloadUsers()
                        }
                    }
                }
            }

            // 3. Segmented Filter Bar (5 options)
            Rectangle {
                Layout.fillWidth: true
                height: 48
                radius: 8
                color: Theme.bgApp
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6

                    Repeater {
                        model: ["Tất cả", "Học viên", "Gia sư", "Đã khóa"]

                        SegmentedFilterButton {
                            text: modelData
                            active: index === adminUsersView.activeFilterIndex
                            onClicked: {
                                adminUsersView.activeFilterIndex = index
                                adminUsersView.applyFilter(index)
                            }
                        }
                    }
                }
            }

            // 4. User Cards List
            ListView {
                id: userListView
                Layout.fillWidth: true
                implicitHeight: Math.max(300, count * 78)
                clip: true
                spacing: 10
                model: adminUsersView.usersModel

                delegate: Rectangle {
                    width: userListView.width
                    height: 72
                    color: Theme.cardBg
                    radius: 8
                    border.color: Theme.cardBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 14

                        // User Avatar Initial Circle
                        Rectangle {
                            width: 40
                            height: 40
                            radius: 20
                            color: (modelData.role === "Quản trị viên" || modelData.role === "Admin") ? Theme.sidebarBg :
                                   (modelData.role === "Gia sư") ? Theme.sidebarActive : Theme.accentBlue

                            Text {
                                anchors.centerIn: parent
                                text: (!modelData.fullName || modelData.fullName === "") ? "U" : modelData.fullName.substring(0, 1).toUpperCase()
                                font.bold: true
                                font.pixelSize: 16
                                color: (modelData.role === "Quản trị viên" || modelData.role === "Admin") ? Theme.textLight : Theme.textDark
                            }
                        }

                        // User Name & Role & Email Info
                        ColumnLayout {
                            Layout.preferredWidth: 200
                            spacing: 2

                            RowLayout {
                                spacing: 8
                                Text {
                                    text: modelData.fullName
                                    font.bold: true
                                    font.pixelSize: 13
                                    color: Theme.textDark
                                    elide: Text.ElideRight
                                }

                                Rectangle {
                                    height: 18
                                    implicitWidth: roleText.implicitWidth + 12
                                    radius: 4
                                    color: (modelData.role === "Quản trị viên" || modelData.role === "Admin") ? Theme.sidebarBg :
                                           (modelData.role === "Gia sư") ? Theme.sidebarActive : Theme.bgApp
                                    border.color: Theme.cardBorder
                                    border.width: 1

                                    Text {
                                        id: roleText
                                        anchors.centerIn: parent
                                        text: modelData.role
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: (modelData.role === "Quản trị viên" || modelData.role === "Admin") ? Theme.textLight : Theme.textDark
                                    }
                                }
                            }

                            Text {
                                text: (modelData.email ? modelData.email : "") + (modelData.username ? " (@" + modelData.username + ")" : "")
                                font.pixelSize: 11
                                color: Theme.textMuted
                                elide: Text.ElideRight
                            }

                            Text {
                                visible: (modelData.role === "Gia sư" && modelData.subject && modelData.subject !== "") || (modelData.role === "Học viên" && modelData.subject && modelData.subject !== "")
                                text: modelData.role === "Gia sư" ? ("Chuyên môn: " + modelData.subject) : ("Môn yêu cầu: " + modelData.subject)
                                font.pixelSize: 11
                                color: Theme.textMuted
                                elide: Text.ElideRight
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Status Badges
                        RowLayout {
                            spacing: 8

                            // Active / Locked Badge
                            Rectangle {
                                implicitWidth: activeBadgeText.implicitWidth + 16
                                height: 26
                                radius: 4
                                color: modelData.isActive ? "#e6f7ff" : "#fff1f0"
                                border.color: modelData.isActive ? "#91d5ff" : "#ffa39e"
                                border.width: 1

                                Text {
                                    id: activeBadgeText
                                    anchors.centerIn: parent
                                    text: modelData.isActive ? "HOẠT ĐỘNG" : "ĐÃ BỊ KHÓA"
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: modelData.isActive ? "#096dd9" : "#cf1322"
                                }
                            }
                        }

                        // Context-Aware Action Buttons
                        RowLayout {
                            spacing: 8

                            // View Profile Detail Button
                            Button {
                                text: "Xem hồ sơ chi tiết"
                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    implicitWidth: 125
                                    implicitHeight: 30
                                    radius: 4
                                    color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                                    border.color: Theme.cardBorder
                                    border.width: 1
                                }
                                onClicked: {
                                    userDetailModal.openForUser(modelData.id)
                                }
                            }

                            // Lock / Unlock Button
                            Button {
                                visible: (modelData.role !== "Quản trị viên" && modelData.role !== "Admin")
                                text: modelData.isActive ? "Khóa TK" : "Mở khóa"
                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: modelData.isActive ? "#cf1322" : Theme.textLight
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    implicitWidth: 76
                                    implicitHeight: 30
                                    radius: 4
                                    color: modelData.isActive ? (parent.hovered ? "#ffa39e" : "#fff1f0")
                                                              : (parent.hovered ? Theme.cSilverLakeBlue : Theme.sidebarBg)
                                    border.color: modelData.isActive ? "#ffa39e" : Theme.sidebarBg
                                    border.width: 1
                                }
                                onClicked: {
                                    if (typeof guiBridge !== "undefined") {
                                        guiBridge.qmlToggleUserLock(modelData.id)
                                        adminUsersView.reloadUsers()
                                    }
                                }
                            }
                        }
                    }
                }

                // Empty State Placeholder
                Rectangle {
                    anchors.centerIn: parent
                    visible: adminUsersView.usersModel.length === 0
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "Không tìm thấy người dùng phù hợp với tìm kiếm / bộ lọc"
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                }
            }
        }
    }

    // Modal Instance
    UserDetailModal {
        id: userDetailModal
    }
}
