INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/lib/Release -llibradar_analysis
Radar_install.files += $$PWD/bin/Release/radar_analysis.dll


Radar_install.path = $$IDE_APP_PATH/
Radar_config_install.files += $$PWD/AsterixSpecification/*
Radar_config_install.path += $$IDE_APP_PATH/AsterixSpecification/
INSTALLS += Radar_install
INSTALLS += Radar_config_install
