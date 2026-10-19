pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    readonly property var blueprintRows: ["00 │ [EDGE]    TLS 1.3 Termination & WAF", "01 │ [INGRESS] Envoy L7 Load Balancer", "02 │ [AUTH]    OIDC Token Validator", "03 │ [LIMIT]   Token-Bucket Rate Limiter", "04 │ [ROUTER]  gRPC Mesh Gateway v2.4", "05 │ [SVC-A]   Order Intake Service", "06 │ [SVC-B]   Payment Ledger Engine", "07 │ [SVC-C]   Fraud Scoring Worker", "08 │ [SVC-D]   Inventory Reservation", "09 │ [CACHE-1] Redis Cluster Shard 01", "10 │ [CACHE-2] Redis Cluster Shard 02", "11 │ [QUEUE-1] Kafka Queue: orders.raw", "12 │ [QUEUE-2] Kafka Queue: payments.tx", "13 │ [STREAM]  Flink Window Aggregator", "14 │ [MESH]    mTLS Sidecar Proxy Pool", "15 │ [TRACE]   OpenTelemetry Collector", "16 │ [METRIC]  Prometheus Scraper Node", "17 │ [ALERT]   PagerDuty Rule Evaluator", "18 │ [DB-PRI]  PostgreSQL 17 Primary", "19 │ [DB-REP]  PostgreSQL Read Replica", "20 │ [POOL]    PgBouncer Connection Pool", "21 │ [SEARCH]  OpenSearch Index Cluster", "22 │ [BLOB]    S3 Object Archive Bucket", "23 │ [CDN]     Anycast Edge PoP Cache", "24 │ [KMS]     HSM Envelope Key Vault", "25 │ [AUDIT]   Immutable WORM Audit Log", "26 │ [BACKUP]  Hourly Snapshot Scheduler", "27 │ [DISK]    NVMe WAL Journal Sync"]

    function jumpTo(targetY) {
        blueprintScroll.contentY = targetY;
        blueprintScroll.focus = true;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "ScrollView Blueprint"
                    color: "#67e8f9"
                }
                Text {
                    text: "[Core]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: "Y=" + blueprintScroll.contentY + "/" + blueprintScroll.contentHeight
                color: "#2cde85"
            }
            footer: Text {
                text: "↑↓/PgUp/PgDn scroll · Home/End"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "ScrollView"
                color: "#8b949e"
            }
            ScrollView {
                id: blueprintScroll

                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                contentHeight: 28
                scrollBarVisible: true

                Column {
                    x: 1
                    y: 0
                    spacing: 0

                    Repeater {
                        model: root.blueprintRows
                        delegate: Text {
                            required property int index
                            required property string modelData

                            text: modelData
                            color: index % 2 === 0 ? "#f0f6fc" : "#58a6ff"
                            bold: index === 0 || index === 27
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            title: Text {
                text: "Inspector"
                color: "#58a6ff"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 2
                }
                spacing: 1

                Text {
                    text: "contentY: " + blueprintScroll.contentY
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "contentHeight: " + blueprintScroll.contentHeight
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "viewportH: " + blueprintScroll.height
                    color: "#c9d1d9"
                }

                Text {
                    text: "Jump Presets:"
                    color: "#8b949e"
                }

                Row {
                    width: implicitWidth
                    height: implicitHeight
                    spacing: 1

                    Button {
                        id: topBtn

                        text: "Top"
                        textColor: "#2cde85"
                        onClicked: root.jumpTo(0)
                    }

                    Button {
                        id: midBtn

                        text: "Mid"
                        textColor: "#58a6ff"
                        onClicked: root.jumpTo(8)
                    }

                    Button {
                        id: endBtn

                        text: "End"
                        textColor: "#fbbf24"
                        onClicked: root.jumpTo(28)
                    }
                }
            }
        }
    }
}
