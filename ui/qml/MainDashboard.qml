import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Item {
    id: mainDashboard
    signal logoutRequested()

    property int currentTabIndex: 0
    property string userRole: (typeof guiBridge !== "undefined" && guiBridge.currentUserRole !== "") ? guiBridge.currentUserRole : "Học viên"

    onUserRoleChanged: {
        currentTabIndex = 0
    }

    onCurrentTabIndexChanged: {
        if (typeof guiBridge !== "undefined") {
            guiBridge.clearStatusMessage()
        }
    }

    function isAdminRole() {
        return userRole === "Quản trị viên" || userRole === "Admin"
    }

    function isTutorRole() {
        return userRole === "Gia sư" || userRole === "Tutor"
    }

    function isStudentRole() {
        return !isAdminRole() && !isTutorRole()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bgApp

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // 1. Sidebar Navigation Container (Left)
            Rectangle {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                color: Theme.sidebarBg

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 20

                    // Brand Logo
                    RowLayout {
                        spacing: 10
                        Rectangle {
                            width: 32
                            height: 32
                            radius: 6
                            color: Theme.sidebarActive
                            Text {
                                anchors.centerIn: parent
                                text: "TM"
                                font.bold: true
                                color: Theme.sidebarActiveText
                            }
                        }
                        Text {
                            text: "TutorMatch"
                            font.bold: true
                            font.pixelSize: 18
                            color: Theme.textLight
                        }
                    }

                    // User Profile Card
                    Rectangle {
                        Layout.fillWidth: true
                        height: 65
                        radius: 8
                        color: "#2E4770"
                        border.color: Theme.sidebarActive
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12

                            Rectangle {
                                width: 40
                                height: 40
                                radius: 20
                                color: Theme.sidebarActive
                                Text {
                                    anchors.centerIn: parent
                                    text: (typeof guiBridge !== "undefined" && guiBridge.currentUserFullName !== "") ? guiBridge.currentUserFullName.substring(0, 1).toUpperCase() : "U"
                                    font.bold: true
                                    color: Theme.sidebarActiveText
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: (typeof guiBridge !== "undefined") ? guiBridge.currentUserFullName : "User Name"
                                    font.bold: true
                                    font.pixelSize: 13
                                    color: Theme.textLight
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: 140
                                }
                                Text {
                                    text: mainDashboard.userRole
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.sidebarActive
                                }
                            }
                        }
                    }

                    // Navigation Menu Repeater List
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: mainDashboard.isAdminRole()
                                   ? ["Tổng quan nền tảng", "Quản lý & Xác minh", "Giám sát Kết nối", "Kiểm duyệt Đánh giá", "Đăng xuất"]
                                   : mainDashboard.isTutorRole()
                                     ? ["Tổng quan & Lịch dạy", "Yêu cầu kết nối", "Học viên & Khóa học", "Đánh giá từ học viên", "Đăng xuất"]
                                     : ["Hồ sơ & Thời khóa biểu", "Tìm kiếm Gia sư", "Gợi ý Thông minh", "Lịch học & Kết nối", "Đăng xuất"]

                            Rectangle {
                                Layout.fillWidth: true
                                height: 40
                                radius: 6
                                color: (index === mainDashboard.currentTabIndex) ? "#2E4770" : "transparent"

                                Rectangle {
                                    width: 4
                                    height: 28
                                    radius: 2
                                    color: Theme.sidebarActive
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: (index === mainDashboard.currentTabIndex)
                                }

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 20
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData
                                    font.bold: (index === mainDashboard.currentTabIndex)
                                    font.pixelSize: 13
                                    color: (index === mainDashboard.currentTabIndex) ? Theme.sidebarActive : Theme.textLight
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        var maxIndex = mainDashboard.isAdminRole() ? 4 :
                                                       mainDashboard.isTutorRole() ? 4 : 4;
                                        if (index === maxIndex) {
                                            mainDashboard.logoutRequested()
                                        } else {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.clearStatusMessage()
                                            }
                                            mainDashboard.currentTabIndex = index
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // 2. Main Content Container (Right)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Topbar Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    color: Theme.cardBg
                    border.color: Theme.cardBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20

                        Text {
                            text: mainDashboard.isAdminRole() ? "BẢNG ĐIỀU KHIỂN QUẢN TRỊ" :
                                  mainDashboard.isTutorRole() ? "BẢNG ĐIỀU KHIỂN GIA SƯ" : "BẢNG ĐIỀU KHIỂN HỌC VIÊN"
                            font.bold: true
                            font.pixelSize: 18
                            color: Theme.textDark
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            width: 80
                            height: 30
                            radius: 6
                            color: Theme.accentBlue
                            Text {
                                anchors.centerIn: parent
                                text: "ONLINE"
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                            }
                        }
                    }
                }

                // Dynamic View Switcher Container (StackLayout)
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 20
                    currentIndex: mainDashboard.currentTabIndex

                    // View Slot 0
                    Loader {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        sourceComponent: mainDashboard.isAdminRole() ? adminTab0Component :
                                         mainDashboard.isTutorRole() ? tutorTab0Component : studentTab0Component
                    }

                    // View Slot 1
                    Loader {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        sourceComponent: mainDashboard.isAdminRole() ? adminTab1Component :
                                         mainDashboard.isTutorRole() ? tutorTab1Component : studentTab1Component
                    }

                    // View Slot 2
                    Loader {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        sourceComponent: mainDashboard.isAdminRole() ? adminTab2Component :
                                         mainDashboard.isTutorRole() ? tutorTab2Component : studentTab2Component
                    }

                    // View Slot 3
                    Loader {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        sourceComponent: mainDashboard.isAdminRole() ? adminTab3Component :
                                         mainDashboard.isTutorRole() ? tutorTab3Component : studentTab3Component
                    }
                }
            }
        }
    }

    // Component Registry
    Component {
        id: defaultEmptyComponent
        Item {}
    }

    // Student Views
    Component {
        id: studentTab0Component
        StudentOverviewView {}
    }

    Component {
        id: studentTab1Component
        StudentTutorSearchView {}
    }

    Component {
        id: studentTab2Component
        StudentSmartMatchView {}
    }

    Component {
        id: studentTab3Component
        StudentBookingsView {
            onNavigateToTabRequested: function(tabIndex) {
                mainDashboard.currentTabIndex = tabIndex
            }
        }
    }

    // Tutor Views
    Component {
        id: tutorTab0Component
        TutorOverview {
            onNavigateToTabRequested: function(tabIndex) {
                mainDashboard.currentTabIndex = tabIndex
            }
        }
    }

    Component {
        id: tutorTab1Component
        TutorBookings {}
    }

    Component {
        id: tutorTab2Component
        TutorStudents {}
    }

    Component {
        id: tutorTab3Component
        TutorStatistics {}
    }

    // Admin Views
    Component {
        id: adminTab0Component
        AdminOverviewView {}
    }

    Component {
        id: adminTab1Component
        AdminUsersView {}
    }

    Component {
        id: adminTab2Component
        AdminMatchesView {}
    }

    Component {
        id: adminTab3Component
        AdminReviewsView {}
    }
}
