# Fujitsubame Finance

**Fujitsubame Finance** is a professional finance dashboard application designed and developed for internal use by Fuji Tsubame Co., Ltd. and its franchises. This desktop application simplifies the management of partner company records and their associated contractual revenues. The solution is built using Qt (QML and C++) and provides a fully localized, responsive, and offline-capable interface.

## Features

- **Company Management**
  - Add, edit, and delete partner companies.
  - Filterable search interface for ease of access.

- **Contract Definition and Revenue Modeling**
  - Configure contract parameters such as revenue thresholds, delays, and multi-payment options.
  - Supports percentage-based payment allocation and optional formula input.
  - Inline validation ensures data quality.

- **Revenue Analysis**
  - Automated revenue breakdown calculations based on contract definitions.
  - Yearly financial summaries viewable per month and per company.
  - Export summaries to CSV format for further reporting.

- **Settings and Data Management**
  - Configure and change the local SQLite database path.
  - Refresh or clear all data with appropriate safety prompts.
  - Persistent settings stored via QSettings.

## Architecture

- **Frontend**: QML (Qt Quick Controls 2/6) with Material styling.
- **Backend**: Qt C++ (Models and Controllers)
- **Persistence**: SQLite (local database)
- **Data Binding**: QAbstractListModel, QVariantMap, and Q_INVOKABLE bindings

### Primary UI Components

- `CompaniesPage.qml` – Displays and manages the company list.
- `StatisticsPage.qml` – Contract entry form and revenue parameterization.
- `SumPage.qml` – Displays the annual summary of revenues per company.
- `SettingsPage.qml` – Database location configuration and data control tools.

## Build and Deployment

This project is built using **Qt Creator** and the **qmake** build system.

### Requirements

- Qt 5.15 or newer (including QtQuick, QtSql)
- Qt Creator IDE
- SQLite support

### Build Steps

1. Open the `.pro` file in Qt Creator.
2. Configure the build kit for Desktop (ensure qmake is used).
3. Build and run the application.

The application will automatically initialize its database upon first launch.

## Data and Security

- All data is stored locally in an SQLite database file.
- No internet or network communication is performed.
- To backup your data, use the Settings page to locate the database file and copy it manually.

## License

Copyright 2025 Fuji Tsubame Co., Ltd

All rights reserved.

This software is proprietary and confidential. Unauthorized copying, distribution, modification, or any other use is strictly prohibited without express written permission from the copyright holder.

## Intended Use

This software is intended exclusively for operational use by Fuji Tsubame Co., Ltd. and its franchise entities. It may be sold or licensed to third parties under formal agreements.

## Notes

- A fully localized **Japanese version** of the application is planned and will be made available in a future release.
