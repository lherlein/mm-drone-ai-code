#pragma once

#include "protocol/Packet.hpp"
#include <QWidget>
#include <QLabel>
#include <vector>

namespace drone {
namespace gcu {
namespace ui {

class TelemetryWidget : public QWidget {
    Q_OBJECT

public:
    explicit TelemetryWidget(QWidget* parent = nullptr);
    ~TelemetryWidget();

    void updateTelemetry(const protocol::TelemetryData& telemetry);

private:
    struct TelemetryField {
        QString label;
        QLabel* valueLabel;
        QString unit;
        std::function<QString(const protocol::TelemetryData&)> getValue;
    };

    std::vector<TelemetryField> fields_;

    void setupUi();
    void createFields();
    void updateFields(const protocol::TelemetryData& telemetry);
};

} // namespace ui
} // namespace gcu
} // namespace drone 